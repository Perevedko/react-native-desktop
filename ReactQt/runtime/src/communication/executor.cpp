
/**
 * Copyright (C) 2016, Canonical Ltd.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * Author: Justin McPherson <justin.mcpherson@canonical.com>
 *
 */

#include "executor.h"
#include "bridge.h"

#include <QJsonDocument>
#include <QSharedPointer>

#include <QStandardPaths>

#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QWebEnginePage>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>

#include <QNetworkProxy>

#include <QWebChannel>
#include <QFile>

class ExecutorPrivate : public QObject {
public:
    ExecutorPrivate(Executor* e) : QObject(e), q_ptr(e) {}

    virtual ServerConnection* connection();
    void processRequests();
    bool readPackageHeaderAndAllocateBuffer();
    bool readPackageBodyToBuffer();
    void processRequest(const QByteArray& request,
                        const Executor::ExecuteCallback& callback = Executor::ExecuteCallback());

public slots:
    void readReply();
    bool readCommand();
    void passReceivedDataToCallback(const QByteArray& data);
    void setupStateMachine();

public:
    QQueue<QByteArray> m_requestQueue;
    QQueue<Executor::ExecuteCallback> m_responseQueue;
    QStateMachine* m_machina = nullptr;
    QByteArray m_inputBuffer;
    QSharedPointer<ServerConnection> m_connection = nullptr;
    Executor* q_ptr = nullptr;
};

static QWebEngineView *webEngine = nullptr;
static CustomWebPage *myPage = nullptr;

CustomWebPage* CustomWebPage::instance() {
    return myPage;
}

void CustomWebPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineNumber, const QString& sourceID) {
    qDebug() << "New message received: " << message << " level: " << level <<" lineNumber: " << lineNumber << " sourceID: " << sourceID;
    if (level == QWebEnginePage::ErrorMessageLevel || message.contains("SecurityError")) {
        qDebug() << "Saving to file...";
        myPage->save("errorHTML.txt");
    }
}

#include <QDir>
#include <QEventLoop>

QString tmpDirPath()
    {
        static QString tmpd = QDir::tempPath() + "/tst_qwebenginepage-"
            + QDateTime::currentDateTime().toString(QLatin1String("yyyyMMddhhmmss"));
        return tmpd;
}



Executor::Executor(ServerConnection* conn, QObject* parent) : IExecutor(parent), d_ptr(new ExecutorPrivate(this)) {
    Q_ASSERT(conn);
    Q_D(Executor);
    d->m_connection = QSharedPointer<ServerConnection>(conn);
    connect(d->connection(), &ServerConnection::dataReady, d, &ExecutorPrivate::readReply);

    qRegisterMetaType<Executor::ExecuteCallback>();
    qRegisterMetaType<QNetworkProxy>("QNetworkProxy");
    qRegisterMetaType<QAbstractSocket::SocketError>();

    webEngine = new QWebEngineView();

    /*webEngine->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    webEngine->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    webEngine->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    webEngine->settings()->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    webEngine->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    webEngine->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    webEngine->settings()->setAttribute(QWebEngineSettings::AutoLoadImages,true);

    webEngine->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,true);
    webEngine->settings()->setAttribute(QWebEngineSettings::XSSAuditingEnabled,true);
    webEngine->settings()->setAttribute(QWebEngineSettings::SpatialNavigationEnabled,true);
    webEngine->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,true);
    webEngine->settings()->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled,true);
    webEngine->settings()->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled,true);

    webEngine->settings()->setAttribute(QWebEngineSettings::AllowRunningInsecureContent,true);
    webEngine->settings()->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript,true);
    webEngine->settings()->setAttribute(QWebEngineSettings::JavascriptCanPaste,true);*/



    auto path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    qDebug() << "setPersistentStoragePath: " << path;

    auto *profile = new QWebEngineProfile("StatusReactProfile", webEngine);

    QDir storageDir(path);
    storageDir.mkdir("WebEngineTest1s");

    profile->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    profile->setPersistentStoragePath(path + "/WebEngineTest1s");
    profile->setCachePath(path + "/WebEngineTest1s");

    /*profile->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    profile->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    profile->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    profile->settings()->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    profile->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    profile->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    profile->settings()->setAttribute(QWebEngineSettings::AutoLoadImages,true);

    profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,true);
    profile->settings()->setAttribute(QWebEngineSettings::XSSAuditingEnabled,true);
    profile->settings()->setAttribute(QWebEngineSettings::SpatialNavigationEnabled,true);
    profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,true);
    profile->settings()->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled,true);
    profile->settings()->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled,true);

    profile->settings()->setAttribute(QWebEngineSettings::AllowRunningInsecureContent,true);
    profile->settings()->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript,true);
    profile->settings()->setAttribute(QWebEngineSettings::JavascriptCanPaste,true);*/

    //profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    //profile->settings()->setAttribute()

    QFile webChannelJsFile(":/qtwebchannel/qwebchannel.js");
    if(!webChannelJsFile.open(QIODevice::ReadOnly) )
    {
        //qFatal( QString("Couldn't open qwebchannel.js file: %1").arg(webChannelJsFile.errorString()) );
    }
    else
    {
        QByteArray webChannelJs = webChannelJsFile.readAll();
        /*webChannelJs.append(
                 "\n"
                 "console.log('!!!! Test message');\n"
                 "new QWebChannel(qt.webChannelTransport, function(channel) {"
                 "   console.log('!!! new QWebChannel is created');"
                 "   var JSobject = channel.objects.RealmTest;"
                 "   console.log('Custom JSObject= ' + JSobject);"
                 "});"
        );*/

        QWebEngineScript script;
            script.setSourceCode(webChannelJs);
            script.setName("qwebchannel.js");
            script.setWorldId(QWebEngineScript::MainWorld);
            script.setInjectionPoint(QWebEngineScript::DocumentCreation);
            script.setRunsOnSubFrames(false);
        profile->scripts()->insert(script);
    }

    myPage = new CustomWebPage(profile, webEngine);

    QWebChannel* channel = new QWebChannel(this);
    myPage->setWebChannel(channel);

    RealmClass* realmInstance = new RealmClass(this);

    channel->registerObject(QStringLiteral("RealmTest"), realmInstance);

    QEventLoop* eventLoop = new QEventLoop();



    /*QFile apiFile(":/qtwebchannel/qwebchannel.js");
    if(!apiFile.open(QIODevice::ReadOnly))
        qDebug()<<"Couldn't load Qt's QWebChannel API!";
    QString apiScript = QString::fromLatin1(apiFile.readAll());
    myPage->runJavaScript(apiScript, 0);*/

    connect(myPage, &QWebEnginePage::loadFinished, this, [=](bool finished) {
        qDebug() << "!!! load finished with result: " << finished;
        eventLoop->exit();
    });

    myPage->setHtml(QString("<html> <head>\n"
                            "         <script type=\"text/javascript\">\n"
                            "           window.onload = function() {\n"
                            "               console.log('! ! ! ! Test message');\n"
                            "               new QWebChannel(qt.webChannelTransport, function(channel) { \n"
                            "               console.log('!!! new QWebChannel is created');\n"
                            "               window.Realm = channel.objects.RealmTest;\n"
                            "               console.log('Custom JSObject= ' + window.Realm);\n"
                            "           }\n);"
                            "          }\n"
                            "         </script>\n"
                            "       </head>\n"
                            "       <body> </body> </html>"
                            ), QUrl("http://wwww.example.com"));
    eventLoop->exec();

    //myPage->setUrl(QUrl("http://google.com"));


    //myPage->load(QUrl("http://google.com"));

    //myPage->setHtml("<html><header><title>This is title</title></header><body>Hello world</body></html>");

    //webEngine->setPage(myPage);
    //
}

Executor::~Executor() {
    resetConnection();
}

void ExecutorPrivate::setupStateMachine() {
    m_machina = new QStateMachine(this);

    QState* initialState = new QState();
    QState* errorState = new QState();
    QState* readyState = new QState();

    initialState->addTransition(connection(), SIGNAL(connectionReady()), readyState);
    readyState->addTransition(connection(), SIGNAL(connectionError()), errorState);

    connect(initialState, &QAbstractState::entered, [=] { connection()->openConnection(); });
    connect(readyState, &QAbstractState::entered, [=] { processRequests(); });
    connect(errorState, &QAbstractState::entered, [=] { m_machina->stop(); });

    m_machina->addState(initialState);
    m_machina->addState(errorState);
    m_machina->addState(readyState);
    m_machina->setInitialState(initialState);
}

void Executor::injectJson(const QString& name, const QVariant& data) {
    QJsonDocument doc = QJsonDocument::fromVariant(data);
    //d_ptr->processRequest(name.toLocal8Bit() + "=" + doc.toJson(QJsonDocument::Compact) + ";");

    myPage->runJavaScript(name.toLocal8Bit() + "=" + doc.toJson(QJsonDocument::Compact) + ";", 0, [](const QVariant &v) {
        qDebug() << "Result of runJavaScript: ";
        if (v.isValid()) {
            qDebug() << v;
        }
    });
}

void Executor::executeApplicationScript(const QByteArray& script, const QUrl& /*sourceUrl*/) {
    //d_ptr->processRequest(script, [=](const QJsonDocument&) { Q_EMIT applicationScriptDone(); });

    //webEngine = new QWebEngineView();
    // QWebEngineProfile *profile = new QWebEngineProfile("MyWebChannelProfile", webEngine);

    /*QWebEngineScript script2;
    script2.setSourceCode(script);
    script2.setName("reactnativeapp.js");
    script2.setWorldId(QWebEngineScript::MainWorld);
    script2.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script2.setRunsOnSubFrames(true);
    profile->scripts()->insert(script2);*/

    //myPage = new QWebEnginePage(webEngine);
    //webEngine->setPage(myPage);
    myPage->runJavaScript("console.log(\"Test msg!!!!\");", 0);
    myPage->runJavaScript(script, 0, [=](const QVariant &v) {
        qDebug() << "Result of runJavaScript:";
        if (v.isValid()) {
            qDebug() << v;
        }
        qDebug() << "Emitting applicationScriptDone...";
        Q_EMIT applicationScriptDone();
    });
}

void Executor::executeJSCall(const QString& method,
                             const QVariantList& args,
                             const Executor::ExecuteCallback& callback) {

    QByteArrayList stringifiedArgs;
    for (const QVariant& arg : args) {
        if (arg.type() == QVariant::List || arg.type() == QVariant::Map) {
            QJsonDocument doc = QJsonDocument::fromVariant(arg);
            stringifiedArgs += doc.toJson(QJsonDocument::Compact);
        } else {
            stringifiedArgs += '"' + arg.toString().toLocal8Bit() + '"';
        }
    }

    //d_ptr->processRequest(
        //QByteArray("__fbBatchedBridge.") + method.toLocal8Bit() + "(" + stringifiedArgs.join(',') + ");", callback);

    myPage->runJavaScript(QByteArray("__fbBatchedBridge.") + method.toLocal8Bit() + "(" + stringifiedArgs.join(',') + ");", 0, [=](const QVariant &v) {
        qDebug() << "Result of runJavaScript: ";
        if (v.isValid()) {
            qDebug() << v;
        }

        QJsonDocument doc;
        if (v != "undefined") {
            doc = QJsonDocument::fromVariant(v);
        }
        callback(doc);
    });
}

ServerConnection* ExecutorPrivate::connection() {
    Q_ASSERT(m_connection);
    return m_connection.data();
}

void Executor::init() {
    d_ptr->setupStateMachine();
    d_ptr->m_machina->start();
}

void Executor::resetConnection() {
    d_ptr->connection()->device()->close();
}

void ExecutorPrivate::processRequests() {
    if (!connection()->isReady() || m_requestQueue.isEmpty()) {
        return;
    }

    QByteArray request = m_requestQueue.dequeue();
    quint32 length = request.size();
    connection()->device()->write((const char*)&length, sizeof(length));
    connection()->device()->write(request.constData(), request.size());
}

bool ExecutorPrivate::readPackageHeaderAndAllocateBuffer() {
    if (m_inputBuffer.capacity() == 0) {
        quint32 length = 0;
        if (connection()->device()->bytesAvailable() < sizeof(length)) {
            return false;
        }
        connection()->device()->read((char*)&length, sizeof(length));
        m_inputBuffer.reserve(length);
    }
    return true;
}

bool ExecutorPrivate::readPackageBodyToBuffer() {
    int toRead = m_inputBuffer.capacity() - m_inputBuffer.size();
    QByteArray read = connection()->device()->read(toRead);
    m_inputBuffer += read;

    if (m_inputBuffer.size() < m_inputBuffer.capacity())
        return false;

    return true;
}

void ExecutorPrivate::readReply() {
    while (readCommand()) {
        ;
    }
}

bool ExecutorPrivate::readCommand() {

    if (!readPackageHeaderAndAllocateBuffer())
        return false;

    if (!readPackageBodyToBuffer())
        return false;

    q_ptr->commandReceived(m_inputBuffer.length());
    passReceivedDataToCallback(m_inputBuffer);
    m_inputBuffer.clear();
    return true;
}

void ExecutorPrivate::passReceivedDataToCallback(const QByteArray& data) {
    if (m_responseQueue.size()) {
        Executor::ExecuteCallback callback = m_responseQueue.dequeue();
        if (callback) {
            QJsonDocument doc;
            if (data != "undefined") {
                doc = QJsonDocument::fromJson(data);
            }
            callback(doc);
        }
    }
}

void ExecutorPrivate::processRequest(const QByteArray& request, const Executor::ExecuteCallback& callback) {

    m_requestQueue.enqueue(request);
    m_responseQueue.enqueue(callback);
    processRequests();
}
