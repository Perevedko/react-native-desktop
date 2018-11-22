
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

#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <QByteArray>
#include <QIODevice>
#include <QQueue>
#include <QStateMachine>

#include "iexecutor.h"
#include "serverconnection.h"

class RealmClass : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool empty READ empty WRITE setEmpty NOTIFY emptyChanged)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(int schemaVersion READ schemaVersion WRITE setSchemaVersion NOTIFY schemaVersionChanged)
    Q_PROPERTY(bool schema READ schema WRITE setSchema NOTIFY schemaChanged)
    Q_PROPERTY(bool inMemory READ inMemory WRITE setInMemory NOTIFY inMemoryChanged)
    Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(bool isInTransaction READ isInTransaction WRITE setIsInTransaction NOTIFY isInTransactionChanged)
    Q_PROPERTY(bool isClosed READ isClosed WRITE setIsClosed NOTIFY isClosedChanged)

    Q_PROPERTY(QString defaultPath READ defaultPath WRITE setDefaultPath NOTIFY defaultPathChanged)
public:

    explicit RealmClass(QObject* parent) : QObject(parent) {}

    bool empty() const { qDebug() << "+++ RealmClass method call"; return true;}
    void setEmpty(bool value) {qDebug() << "+++ RealmClass method call";}

    QString path() const {qDebug() << "+++ RealmClass method call"; return "";}
    void setPath(QString value) {qDebug() << "+++ RealmClass method call";}

    Q_INVOKABLE static int schemaVersion() {qDebug() << "+++ RealmClass method call"; return 0;}
    void setSchemaVersion(int value) {qDebug() << "+++ RealmClass method call";}

    bool schema() const {qDebug() << "+++ RealmClass method call"; return true;}
    void setSchema(bool value) {qDebug() << "+++ RealmClass method call";}

    bool inMemory() const {qDebug() << "+++ RealmClass method call"; return true;}
    void setInMemory(bool value) {qDebug() << "+++ RealmClass method call";}

    bool readOnly() const {qDebug() << "+++ RealmClass method call"; return true;}
    void setReadOnly(bool value) {qDebug() << "+++ RealmClass method call";}

    bool isInTransaction() const {qDebug() << "+++ RealmClass method call"; return true;}
    void setIsInTransaction(bool value) {qDebug() << "+++ RealmClass method call";}

    bool isClosed() const {qDebug() << "+++ RealmClass method call"; return true;}
    void setIsClosed(bool value) {qDebug() << "+++ RealmClass method call";}

    QString defaultPath() const {qDebug() << "+++ RealmClass defaultPath method call"; return "";}
    void setDefaultPath(QString value) {qDebug() << "+++ RealmClass setDefaultPath method call";}

Q_SIGNALS:

    void emptyChanged();
    void pathChanged();
    void schemaVersionChanged();
    void schemaChanged();
    void inMemoryChanged();
    void readOnlyChanged();
    void isInTransactionChanged();
    void isClosedChanged();
    void defaultPathChanged();

};

class ExecutorPrivate;
class Executor : public IExecutor {
    Q_OBJECT
    Q_DECLARE_PRIVATE(Executor)

public:
    typedef std::function<void(const QJsonDocument&)> ExecuteCallback;

    Executor(ServerConnection* conn, QObject* parent = nullptr);
    ~Executor();

    Q_INVOKABLE virtual void init();
    Q_INVOKABLE virtual void resetConnection();

    Q_INVOKABLE virtual void injectJson(const QString& name, const QVariant& data);
    Q_INVOKABLE virtual void executeApplicationScript(const QByteArray& script, const QUrl& sourceUrl);
    Q_INVOKABLE virtual void executeJSCall(const QString& method,
                                           const QVariantList& args = QVariantList(),
                                           const Executor::ExecuteCallback& callback = ExecuteCallback());

private:
    QScopedPointer<ExecutorPrivate> d_ptr;
};

Q_DECLARE_METATYPE(Executor::ExecuteCallback)

#endif // EXECUTOR_H
