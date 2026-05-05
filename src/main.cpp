#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QKeySequence>
#include <QLabel>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMenu>
#include <QMenuBar>
#include <QProcess>
#include <QRect>
#include <QScreen>
#include <QStandardPaths>
#include <QTextStream>
#include <QTextCursor>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

namespace {

// Small app state bundle for the quick local viewer.
struct AppState {
    QWidget* window = nullptr;
    QTextBrowser* browser = nullptr;
    QLabel* status_label = nullptr;
    QFileSystemWatcher* watcher = nullptr;
    QLocalServer* control_server = nullptr;
    QString control_server_name;
    QString registry_file_path;
    QString current_path;
    QString pending_anchor;
};

struct CommandLineOptions {
    QString startup_path;
    QString scroll_anchor;
    int monitor_index = -1;
    qint64 target_pid = -1;
    bool show_info = false;
    bool dump_visible = false;
    bool list_instances = false;
};

constexpr const char* kDocumentCss = R"(
body {
    margin: 0;
    background: #f5f0e8;
    color: #1e1a17;
    font-family: 'Noto Serif', 'DejaVu Serif', serif;
    line-height: 1.6;
}

.page {
    max-width: 860px;
    margin: 0 auto;
    padding: 24px 28px 36px;
}

h1, h2, h3, h4, h5, h6 {
    color: #111111;
    font-family: 'Noto Sans', 'DejaVu Sans', sans-serif;
}

h1 {
    border-bottom: 1px solid #d8cfc3;
    padding-bottom: 10px;
}

pre {
    background: #1f1b18;
    color: #f8f0e3;
    padding: 12px;
    border-radius: 8px;
    white-space: pre-wrap;
}

code {
    background: #ece4d9;
    padding: 2px 4px;
    border-radius: 4px;
    font-family: 'DejaVu Sans Mono', monospace;
}

pre code {
    background: transparent;
    padding: 0;
}

blockquote {
    margin: 16px 0;
    padding: 8px 14px;
    border-left: 4px solid #a87536;
    background: #efe6d7;
}

table {
    width: 100%;
    border-collapse: collapse;
}

th, td {
    border: 1px solid #d8cfc3;
    padding: 8px 10px;
}

th {
    background: #ece4d9;
}

a {
    color: #7a2e1f;
}

.welcome, .error {
    border-radius: 10px;
    padding: 16px 18px;
}

.welcome {
    border: 1px dashed #c8b9a8;
    background: #fbf7f1;
}

.error {
    border: 1px solid #c96d5c;
    background: #fff0ed;
    color: #5c2016;
}
)";

QString RuntimeRoot() {
    QString root = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (root.isEmpty()) {
        root = QDir::tempPath();
    }
    return root + "/slime_md_viewer";
}

QString RegistryDirectory() {
    return RuntimeRoot() + "/instances";
}

QString ServerNameForPid(qint64 pid) {
    return QString("slime_md_viewer_%1").arg(pid);
}

QString RegistryFileForPid(qint64 pid) {
    return RegistryDirectory() + "/" + QString::number(pid) + ".instance";
}

struct ViewerInstance {
    qint64 pid = -1;
    QString server_name;
    QString current_path;
    QString registry_path;
};

QString SanitizeVisibleText(QString text) {
    text.replace(QChar(0x2029), "\n");
    text.replace(QChar(0x00A0), " ");
    text.replace(QChar::ObjectReplacementCharacter, "");
    return text.trimmed();
}

bool IsInstanceReachable(const ViewerInstance& instance) {
    if (instance.server_name.isEmpty()) {
        return false;
    }

    QLocalSocket socket;
    socket.connectToServer(instance.server_name);
    const bool connected = socket.waitForConnected(100);
    if (connected) {
        socket.disconnectFromServer();
    }
    return connected;
}

void EnsureRuntimeDirectories() {
    QDir().mkpath(RegistryDirectory());
}

QList<ViewerInstance> LoadViewerInstances() {
    EnsureRuntimeDirectories();

    QList<ViewerInstance> instances;
    QDir registry_dir(RegistryDirectory());
    const QFileInfoList files =
        registry_dir.entryInfoList({"*.instance"}, QDir::Files, QDir::Time | QDir::Reversed);

    for (const QFileInfo& file_info : files) {
        QFile file(file_info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        ViewerInstance instance;
        instance.registry_path = file_info.absoluteFilePath();
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString line = stream.readLine();
            const int equals_index = line.indexOf('=');
            if (equals_index <= 0) {
                continue;
            }

            const QString key = line.left(equals_index).trimmed();
            const QString value = line.mid(equals_index + 1).trimmed();
            if (key == "pid") {
                bool ok = false;
                const qint64 pid = value.toLongLong(&ok);
                if (ok) {
                    instance.pid = pid;
                }
            } else if (key == "server") {
                instance.server_name = value;
            } else if (key == "path") {
                instance.current_path = value;
            }
        }

        if (instance.pid > 0 && !instance.server_name.isEmpty() && IsInstanceReachable(instance)) {
            instances.append(instance);
        } else if (!instance.registry_path.isEmpty()) {
            QFile::remove(instance.registry_path);
        }
    }

    return instances;
}

QString FormatViewerInstance(const ViewerInstance& instance) {
    return QString("pid=%1\npath=%2\nserver=%3\n")
        .arg(instance.pid)
        .arg(instance.current_path.isEmpty() ? "(none)" : instance.current_path)
        .arg(instance.server_name);
}

ViewerInstance PickTargetInstance(qint64 target_pid) {
    const QList<ViewerInstance> instances = LoadViewerInstances();
    if (instances.isEmpty()) {
        return {};
    }

    if (target_pid > 0) {
        for (const ViewerInstance& instance : instances) {
            if (instance.pid == target_pid) {
                return instance;
            }
        }
        return {};
    }

    return instances.last();
}

void WriteInstanceRegistry(const AppState* app_state) {
    if (app_state->registry_file_path.isEmpty() || app_state->control_server_name.isEmpty()) {
        return;
    }

    QFile file(app_state->registry_file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }

    QTextStream stream(&file);
    stream << "pid=" << QCoreApplication::applicationPid() << "\n";
    stream << "server=" << app_state->control_server_name << "\n";
    stream << "path=" << app_state->current_path << "\n";
}

void RemoveInstanceRegistry(const AppState* app_state) {
    if (!app_state->registry_file_path.isEmpty()) {
        QFile::remove(app_state->registry_file_path);
    }
}

// Parse the two tiny command-line knobs we support right now.
CommandLineOptions ParseCommandLine(int argc, char** argv) {
    CommandLineOptions options;
    for (int i = 1; i < argc; ++i) {
        const QString argument = QString::fromLocal8Bit(argv[i]);
        if (argument == "--scroll" && i + 1 < argc) {
            options.scroll_anchor = QString::fromLocal8Bit(argv[++i]);
            continue;
        }
        if (argument == "--monitor" && i + 1 < argc) {
            bool ok = false;
            const int monitor_number = QString::fromLocal8Bit(argv[++i]).toInt(&ok);
            if (ok) {
                options.monitor_index = monitor_number - 1;
            }
            continue;
        }
        if (argument == "--pid" && i + 1 < argc) {
            bool ok = false;
            options.target_pid = QString::fromLocal8Bit(argv[++i]).toLongLong(&ok);
            if (!ok) {
                options.target_pid = -1;
            }
            continue;
        }
        if (argument == "--info") {
            options.show_info = true;
            continue;
        }
        if (argument == "--dump-visible") {
            options.dump_visible = true;
            continue;
        }
        if (argument == "--list-instances") {
            options.list_instances = true;
            continue;
        }
        if (!argument.startsWith("--") && options.startup_path.isEmpty()) {
            options.startup_path = argument;
        }
    }
    return options;
}

// Send a one-shot command to a running viewer instance.
bool SendCommandToRunningViewer(const QString& command, qint64 target_pid, QString* response_text) {
    const ViewerInstance target = PickTargetInstance(target_pid);
    if (target.pid <= 0 || target.server_name.isEmpty()) {
        return false;
    }

    QLocalSocket socket;
    socket.connectToServer(target.server_name);
    if (!socket.waitForConnected(250)) {
        QFile::remove(target.registry_path);
        return false;
    }

    socket.write(command.toUtf8());
    socket.write("\n");
    socket.flush();
    socket.waitForBytesWritten(250);
    if (response_text != nullptr) {
        socket.waitForReadyRead(500);
        *response_text = QString::fromUtf8(socket.readAll());
    }
    socket.disconnectFromServer();
    return true;
}

// Escape plain text before dropping it into an HTML document.
QString EscapeHtml(const QString& text) {
    return text.toHtmlEscaped();
}

// Wrap a markdown fragment into one styled HTML document for QTextBrowser.
QString WrapHtmlDocument(const QString& title, const QString& body_html) {
    return "<!doctype html>\n"
           "<html>\n"
           "<head>\n"
           "  <meta charset=\"utf-8\">\n"
           "  <style>\n" +
           QString::fromUtf8(kDocumentCss) + "\n  </style>\n"
           "  <title>" + EscapeHtml(title) + "</title>\n"
           "</head>\n"
           "<body>\n"
           "  <main class=\"page\">\n" + body_html + "\n  </main>\n"
           "</body>\n"
           "</html>\n";
}

// Run one local parser command and capture both its output and any useful error text.
bool RunProcessCapture(
    const QString& program,
    const QStringList& arguments,
    QString* stdout_text,
    QString* stderr_text) {
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.start();

    if (!process.waitForStarted(3000)) {
        if (stderr_text != nullptr) {
            *stderr_text = "Failed to start " + program;
        }
        return false;
    }

    if (!process.waitForFinished(15000)) {
        process.kill();
        process.waitForFinished(1000);
        if (stderr_text != nullptr) {
            *stderr_text = program + " timed out.";
        }
        return false;
    }

    if (stdout_text != nullptr) {
        *stdout_text = QString::fromUtf8(process.readAllStandardOutput());
    }
    if (stderr_text != nullptr) {
        *stderr_text = QString::fromUtf8(process.readAllStandardError());
    }

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

// Convert markdown to HTML using local parser tools instead of embedding a markdown library yet.
bool ConvertMarkdownToHtml(const QString& path, QString* html, QString* error_text) {
    QString parser_output;
    QString parser_error;

    if (RunProcessCapture(
            "pandoc",
            {"--from", "gfm", "--to", "html5", path},
            &parser_output,
            &parser_error)) {
        *html = WrapHtmlDocument(QFileInfo(path).fileName(), parser_output);
        return true;
    }

    const QString pandoc_error = parser_error;
    parser_output.clear();
    parser_error.clear();

    if (RunProcessCapture(
            "python3",
            {"-m", "markdown", path},
            &parser_output,
            &parser_error)) {
        *html = WrapHtmlDocument(QFileInfo(path).fileName(), parser_output);
        return true;
    }

    if (error_text != nullptr) {
        *error_text =
            "Neither local parser path worked.\n\npandoc:\n" + pandoc_error +
            "\n\npython3 -m markdown:\n" + parser_error;
    }
    return false;
}

void SetStatus(AppState* app_state, const QString& status) {
    app_state->status_label->setText(status);
    WriteInstanceRegistry(app_state);
}

// Scroll to a known HTML anchor once the document is loaded.
void ScrollToAnchor(AppState* app_state, const QString& anchor) {
    if (anchor.isEmpty()) {
        return;
    }
    app_state->browser->scrollToAnchor(anchor);
    SetStatus(app_state, app_state->current_path.isEmpty()
                             ? QString("Scrolled to #") + anchor
                             : app_state->current_path + "  [#" + anchor + "]");
}

// Replace the watch target with the current markdown file so saves trigger a reload.
void ResetWatcher(AppState* app_state) {
    if (!app_state->watcher->files().isEmpty()) {
        app_state->watcher->removePaths(app_state->watcher->files());
    }

    if (!app_state->current_path.isEmpty()) {
        app_state->watcher->addPath(app_state->current_path);
    }
}

void ShowLocalPage(AppState* app_state, const QString& title, const QString& body_html) {
    app_state->browser->document()->setBaseUrl(QUrl());
    app_state->browser->setSearchPaths({});
    app_state->browser->setHtml(WrapHtmlDocument(title, body_html));
}

QString BuildInfoReport(const AppState* app_state) {
    return QString("pid=%1\npath=%2\ntitle=%3\nserver=%4\n")
        .arg(QCoreApplication::applicationPid())
        .arg(app_state->current_path.isEmpty() ? "(none)" : app_state->current_path)
        .arg(app_state->window->windowTitle())
        .arg(app_state->control_server_name);
}

QString VisibleTextSlice(const AppState* app_state) {
    const QRect rect = app_state->browser->viewport()->rect();
    if (!rect.isValid() || rect.width() <= 0 || rect.height() <= 0) {
        return {};
    }

    const QPoint top_left = rect.topLeft() + QPoint(2, 2);
    const QPoint bottom_right = rect.bottomRight() - QPoint(2, 2);

    QTextCursor start_cursor = app_state->browser->cursorForPosition(top_left);
    QTextCursor end_cursor = app_state->browser->cursorForPosition(bottom_right);
    if (start_cursor.position() > end_cursor.position()) {
        std::swap(start_cursor, end_cursor);
    }

    start_cursor.setPosition(end_cursor.position(), QTextCursor::KeepAnchor);
    return SanitizeVisibleText(start_cursor.selectedText());
}

QString BuildVisibleDump(const AppState* app_state) {
    QString output = BuildInfoReport(app_state) + "\n";
    output += "visible-text:\n";
    const QString visible_text = VisibleTextSlice(app_state);
    if (visible_text.isEmpty()) {
        output += "(empty)\n";
    } else {
        output += visible_text + "\n";
    }
    return output;
}

// Render one markdown file into the QTextBrowser and update path/watch state.
void LoadMarkdownFile(AppState* app_state, const QString& path) {
    QFileInfo info(path);
    if (!info.exists()) {
        ShowLocalPage(
            app_state,
            "Missing File",
            "<section class=\"error\"><h1>File not found</h1><p><code>" +
                EscapeHtml(path) + "</code></p></section>");
        SetStatus(app_state, "Missing file");
        return;
    }

    QString html;
    QString error_text;
    if (!ConvertMarkdownToHtml(path, &html, &error_text)) {
        ShowLocalPage(
            app_state,
            "Render Error",
            "<section class=\"error\"><h1>Could not render markdown</h1><pre>" +
                EscapeHtml(error_text) + "</pre></section>");
        SetStatus(app_state, "Render error");
        return;
    }

    app_state->current_path = info.absoluteFilePath();
    app_state->browser->document()->setBaseUrl(QUrl::fromLocalFile(info.absolutePath() + "/"));
    app_state->browser->setSearchPaths({info.absolutePath()});
    app_state->browser->setHtml(html);
    app_state->window->setWindowTitle(info.fileName());
    SetStatus(app_state, app_state->current_path);
    ResetWatcher(app_state);

    if (!app_state->pending_anchor.isEmpty()) {
        const QString anchor = app_state->pending_anchor;
        QTimer::singleShot(0, app_state->browser, [app_state, anchor]() {
            ScrollToAnchor(app_state, anchor);
        });
    }
}

void ShowWelcomePage(AppState* app_state) {
    ShowLocalPage(
        app_state,
        "SlimeMDViewer",
        "<section class=\"welcome\">"
        "<h1>SlimeMDViewer</h1>"
        "<p>Open a <code>.md</code> file and this thing will render it with a local parser.</p>"
        "<p>Current parser order: <code>pandoc</code> first, then <code>python3 -m markdown</code>.</p>"
        "</section>");
    SetStatus(app_state, "Open a markdown file");
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    const CommandLineOptions options = ParseCommandLine(argc, argv);

    if (options.list_instances) {
        QTextStream out(stdout);
        const QList<ViewerInstance> instances = LoadViewerInstances();
        if (instances.isEmpty()) {
            out << "No running slime instances found.\n";
            return 0;
        }
        for (const ViewerInstance& instance : instances) {
            out << FormatViewerInstance(instance) << "\n";
        }
        return 0;
    }

    if (!options.scroll_anchor.isEmpty() && options.startup_path.isEmpty()) {
        if (SendCommandToRunningViewer("scroll:" + options.scroll_anchor, options.target_pid, nullptr)) {
            return 0;
        }
    }

    if (options.show_info && options.startup_path.isEmpty()) {
        QString response_text;
        if (SendCommandToRunningViewer("info", options.target_pid, &response_text)) {
            QTextStream(stdout) << response_text;
            return 0;
        }
    }

    if (options.dump_visible && options.startup_path.isEmpty()) {
        QString response_text;
        if (SendCommandToRunningViewer("dump-visible", options.target_pid, &response_text)) {
            QTextStream(stdout) << response_text;
            return 0;
        }
    }

    auto state = std::make_shared<AppState>();
    state->window = new QWidget();
    state->window->setWindowTitle("SlimeMDViewer");
    state->window->resize(1100, 780);
    if (options.monitor_index >= 0) {
        const QList<QScreen*> screens = app.screens();
        if (options.monitor_index < screens.size() && screens[options.monitor_index] != nullptr) {
            const QRect available = screens[options.monitor_index]->availableGeometry();
            const QSize size = state->window->size();
            const int x = available.x() + (available.width() - size.width()) / 2;
            const int y = available.y() + (available.height() - size.height()) / 2;
            state->window->move(x, y);
        }
    }

    auto* root_layout = new QVBoxLayout(state->window);
    root_layout->setContentsMargins(10, 10, 10, 10);
    root_layout->setSpacing(8);

    auto* menu_bar = new QMenuBar();
    auto* file_menu = new QMenu("File", menu_bar);
    auto* open_action = file_menu->addAction("Open...");
    open_action->setShortcut(QKeySequence::Open);
    auto* reload_action = file_menu->addAction("Reload");
    reload_action->setShortcut(QKeySequence::Refresh);
    file_menu->addSeparator();
    auto* quit_action = file_menu->addAction("Quit");
    quit_action->setShortcut(QKeySequence::Quit);
    menu_bar->addMenu(file_menu);

    auto* settings_menu = new QMenu("Settings", menu_bar);
    auto* settings_placeholder = settings_menu->addAction("Preferences coming later");
    settings_placeholder->setEnabled(false);
    menu_bar->addMenu(settings_menu);
    root_layout->setMenuBar(menu_bar);

    state->browser = new QTextBrowser();
    state->browser->setOpenExternalLinks(false);
    state->browser->setReadOnly(true);
    state->browser->setStyleSheet(
        "QTextBrowser { background: #f5f0e8; color: #1e1a17; border: none; }");
    root_layout->addWidget(state->browser, 1);

    state->status_label = new QLabel("Starting...");
    state->status_label->setStyleSheet("color: #5f564d;");
    root_layout->addWidget(state->status_label);

    state->watcher = new QFileSystemWatcher(state->window);
    state->control_server = new QLocalServer(state->window);
    state->control_server_name = ServerNameForPid(QCoreApplication::applicationPid());
    state->registry_file_path = RegistryFileForPid(QCoreApplication::applicationPid());

    EnsureRuntimeDirectories();

    QObject::connect(state->control_server, &QLocalServer::newConnection, [state]() {
        while (state->control_server->hasPendingConnections()) {
            QLocalSocket* socket = state->control_server->nextPendingConnection();
            QObject::connect(socket, &QLocalSocket::readyRead, [state, socket]() {
                const QString command = QString::fromUtf8(socket->readAll()).trimmed();
                if (command.startsWith("scroll:")) {
                    const QString anchor = command.mid(QString("scroll:").size()).trimmed();
                    if (!anchor.isEmpty()) {
                        state->pending_anchor = anchor;
                        QTimer::singleShot(0, state->browser, [state, anchor]() {
                            ScrollToAnchor(state.get(), anchor);
                        });
                    }
                } else if (command == "info") {
                    socket->write(BuildInfoReport(state.get()).toUtf8());
                    socket->flush();
                } else if (command == "dump-visible") {
                    socket->write(BuildVisibleDump(state.get()).toUtf8());
                    socket->flush();
                }
                socket->disconnectFromServer();
            });
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
        }
    });

    if (!state->control_server->listen(state->control_server_name)) {
        QLocalSocket probe;
        probe.connectToServer(state->control_server_name);
        if (!probe.waitForConnected(100)) {
            QLocalServer::removeServer(state->control_server_name);
            state->control_server->listen(state->control_server_name);
        }
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [state]() {
        RemoveInstanceRegistry(state.get());
    });

    QObject::connect(open_action, &QAction::triggered, [state]() {
        const QString path = QFileDialog::getOpenFileName(
            state->window,
            "Open Markdown",
            QString(),
            "Markdown Files (*.md *.markdown *.mdown);;All Files (*)");
        if (!path.isEmpty()) {
            LoadMarkdownFile(state.get(), path);
        }
    });

    QObject::connect(reload_action, &QAction::triggered, [state]() {
        if (state->current_path.isEmpty()) {
            ShowWelcomePage(state.get());
            return;
        }
        LoadMarkdownFile(state.get(), state->current_path);
    });

    QObject::connect(quit_action, &QAction::triggered, state->window, &QWidget::close);

    QObject::connect(state->watcher, &QFileSystemWatcher::fileChanged, [state](const QString&) {
        if (state->current_path.isEmpty()) {
            return;
        }
        // Saving via temp-file swap can briefly remove the target, so delay the reload slightly.
        QTimer::singleShot(150, [state]() {
            if (!state->current_path.isEmpty()) {
                LoadMarkdownFile(state.get(), state->current_path);
            }
        });
    });

    QObject::connect(state->browser, &QTextBrowser::anchorClicked, [state](const QUrl& url) {
        if (url.isLocalFile()) {
            const QFileInfo info(url.toLocalFile());
            const QString suffix = info.suffix().toLower();
            if (suffix == "md" || suffix == "markdown" || suffix == "mdown") {
                LoadMarkdownFile(state.get(), info.absoluteFilePath());
                return;
            }
        }
        QDesktopServices::openUrl(url);
    });

    state->pending_anchor = options.scroll_anchor;

    if (!options.startup_path.isEmpty()) {
        LoadMarkdownFile(state.get(), options.startup_path);
    } else {
        ShowWelcomePage(state.get());
    }

    WriteInstanceRegistry(state.get());

    state->window->show();
    return app.exec();
}
