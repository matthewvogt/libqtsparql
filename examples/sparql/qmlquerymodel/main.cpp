/****************************************************************************
**
** Copyright (C) 2010-2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (ivan.frade@nokia.com)
**
** This file is part of QtSparqlTracker library.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at ivan.frade@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/QApplication>
#include <QtDeclarative>
#include <QtSparql>
#include <QtDBus/QtDBus>
#include <QString>

class ModelLiveChange : public QObject
{
    Q_OBJECT
public :
    ModelLiveChange(QSparqlQueryModel *model);
    ~ModelLiveChange();

    QSparqlQueryModel *model;
    QSparqlConnection *connection;
    QString query;

public slots:
    void changed(QString);
    void gotClick(QString, QString);
};

ModelLiveChange::ModelLiveChange(QSparqlQueryModel *model) : model(model)
{
    // The property names in QML will be "u" "firstName" and "secondName"
    query = "select ?u ?firstName ?secondName"
            "{ ?u a nco:PersonContact;"
            "nie:isLogicalPartOf <qml-example>;"
            "nco:nameGiven ?firstName;"
            "nco:nameFamily ?secondName .} order by ?secondName ?firstName";

    // In this example we use the QTRACKER_DIRECT driver, QML model support
    // also works the same for the QSPARQL_ENDPOINT driver.
    connection = new QSparqlConnection("QTRACKER_DIRECT");

    // Create a new model and set the query
    model->setQuery(QSparqlQuery(query), *connection);

    // Now we need to monitor dbus for any notifications from Tracker.
    // More information about Tracker over Dbus can be found at :
    // http://live.gnome.org/Tracker/Documentation/SignalsOnChanges
    // Please note: a more complete implementation, TrackerChangeNotifier,
    // is available in libqtsparql-tracker-extensions
    QString service("org.freedesktop.Tracker1");
    QString basePath("/org/freedesktop/Tracker1");
    QString resourcesInterface("org.freedesktop.Tracker1.Resources");
    QString resourcesPath("/org/freedesktop/Tracker1/Resources");
    QString changedSignal("GraphUpdated");
    QString changedSignature("sa(iiii)a(iiii)");
    QString syncFunction("Sync");
    // We'll need to use the long form of the class name to watch.
    QString className("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#PersonContact");
    QDBusConnection::sessionBus().connect(service, resourcesPath,
                                          resourcesInterface, changedSignal,
                                          QStringList() << className,
                                          changedSignature,
                                          this, SLOT(changed(QString)));
}

ModelLiveChange::~ModelLiveChange()
{
    delete model;
    delete connection;
}

void ModelLiveChange::changed(QString className)
{
    Q_UNUSED(className);
    // We got a change notification from DBus on the class
    // we are watching, so call setQuery on the model again
    // this will make the model requery, and any new results
    // will be displayed
    model->setQuery(QSparqlQuery(query), *connection);
}

void ModelLiveChange::gotClick(QString firstName, QString familyName)
{
    // We use this slot to handel contact inserts from QML, simply insert
    // them and the change notifier will pick them up and requery
    QString insertString = QString("insert { _:u a nco:PersonContact;"
                                   "nie:isLogicalPartOf <qml-example>;"
                                   "nco:nameGiven '%1';"
                                   "nco:nameFamily '%2' .}").arg(firstName).arg(familyName);

    QSparqlQuery insertQuery(insertString, QSparqlQuery::InsertStatement);
    QSparqlResult *r = connection->syncExec(insertQuery);
    delete r;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Initilise QML
    QDeclarativeView viewQml;
    QDeclarativeContext *ctxt = viewQml.rootContext();

    // Create the model
    QSparqlQueryModel *model = new QSparqlQueryModel();
    // Now set the context property for the ListView model to the liveQuery model
    // access the values in qml using urn and subject
    ctxt->setContextProperty("contactModel", model);
    viewQml.setSource(QUrl::fromLocalFile("main.qml"));

    ModelLiveChange changeNotifier(model);

    // When a contact is added in the app, a "addContact" signal is emitted, connect
    // this to a slot so we can insert the data into tracker
    QObject::connect(viewQml.rootObject(), SIGNAL(addContact(QString, QString)),
                     &changeNotifier, SLOT(gotClick(QString, QString)));
    viewQml.show();

    return app.exec();
}
#include "main.moc"

/*
How to insert/delete example data into tracker:
tracker-sparql -qu "insert { _:u a nco:PersonContact; nie:isLogicalPartOf <qml-example>; nco:nameGiven 'foo'; nco:nameFamily 'bar' .}"
tracker-sparql -qu "delete { ?u a nco:PersonContact } WHERE { ?u a nco:PersonContact; nie:isLogicalPartOf <qml-example> }"
*/
