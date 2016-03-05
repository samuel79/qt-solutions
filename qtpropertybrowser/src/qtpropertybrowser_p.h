/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include "qtpropertybrowser.h"

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QtPropertyPrivate
{
public:
    QtPropertyPrivate(QtAbstractPropertyManager *manager)
        : m_enabled(true),
          m_modified(false),
          m_manager(manager) {}
    QtProperty *q_ptr;

    QSet<QtProperty *> m_parentItems;
    QList<QtProperty *> m_subItems;

    QString m_toolTip;
    QString m_statusTip;
    QString m_whatsThis;
    QString m_name;
    bool m_enabled;
    bool m_modified;

    QtAbstractPropertyManager * const m_manager;
};

class QtAbstractPropertyManagerPrivate
{
    QtAbstractPropertyManager *q_ptr;
    Q_DECLARE_PUBLIC(QtAbstractPropertyManager)
public:
    void propertyDestroyed(QtProperty *property);
    void propertyChanged(QtProperty *property) const;
    void propertyRemoved(QtProperty *property,
                QtProperty *parentProperty) const;
    void propertyInserted(QtProperty *property, QtProperty *parentProperty,
                QtProperty *afterProperty) const;

    QSet<QtProperty *> m_properties;
};


////////////////////////////////////
class QtBrowserItemPrivate
{
public:
    QtBrowserItemPrivate(QtAbstractPropertyBrowser *browser, QtProperty *property, QtBrowserItem *parent)
        : m_browser(browser), m_property(property), m_parent(parent), q_ptr(0) {}

    void addChild(QtBrowserItem *index, QtBrowserItem *after);
    void removeChild(QtBrowserItem *index);

    QtAbstractPropertyBrowser * const m_browser;
    QtProperty *m_property;
    QtBrowserItem *m_parent;

    QtBrowserItem *q_ptr;

    QList<QtBrowserItem *> m_children;

};

class QtAbstractPropertyBrowserPrivate
{
    QtAbstractPropertyBrowser *q_ptr;
    Q_DECLARE_PUBLIC(QtAbstractPropertyBrowser)
public:
    QtAbstractPropertyBrowserPrivate();

    void insertSubTree(QtProperty *property,
            QtProperty *parentProperty);
    void removeSubTree(QtProperty *property,
            QtProperty *parentProperty);
    void createBrowserIndexes(QtProperty *property, QtProperty *parentProperty, QtProperty *afterProperty);
    void removeBrowserIndexes(QtProperty *property, QtProperty *parentProperty);
    QtBrowserItem *createBrowserIndex(QtProperty *property, QtBrowserItem *parentIndex, QtBrowserItem *afterIndex);
    void removeBrowserIndex(QtBrowserItem *index);
    void clearIndex(QtBrowserItem *index);

    void slotPropertyInserted(QtProperty *property,
            QtProperty *parentProperty, QtProperty *afterProperty);
    void slotPropertyRemoved(QtProperty *property, QtProperty *parentProperty);
    void slotPropertyDestroyed(QtProperty *property);
    void slotPropertyDataChanged(QtProperty *property);

    QList<QtProperty *> m_subItems;
    QMap<QtAbstractPropertyManager *, QList<QtProperty *> > m_managerToProperties;
    QMap<QtProperty *, QList<QtProperty *> > m_propertyToParents;

    QMap<QtProperty *, QtBrowserItem *> m_topLevelPropertyToIndex;
    QList<QtBrowserItem *> m_topLevelIndexes;
    QMap<QtProperty *, QList<QtBrowserItem *> > m_propertyToIndexes;

    QtBrowserItem *m_currentItem;
};


#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

