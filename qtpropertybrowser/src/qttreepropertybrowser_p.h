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
#include "qttreepropertybrowser.h"

#include <QTreeWidget>
#include <QItemDelegate>
#include <QSet>
#include <QIcon>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QApplication>
#include <QFocusEvent>
#include <QStyle>
#include <QPalette>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QTreeWidgetItem;
class QtPropertyEditorView;

class QtTreePropertyBrowserPrivate
{
    QtTreePropertyBrowser *q_ptr;
    Q_DECLARE_PUBLIC(QtTreePropertyBrowser)

public:
    QtTreePropertyBrowserPrivate();
    void init(QWidget *parent);

    void propertyInserted(QtBrowserItem *index, QtBrowserItem *afterIndex);
    void propertyRemoved(QtBrowserItem *index);
    void propertyChanged(QtBrowserItem *index);
    QWidget *createEditor(QtProperty *property, QWidget *parent) const
        { return q_ptr->createEditor(property, parent); }
    QtProperty *indexToProperty(const QModelIndex &index) const;
    QTreeWidgetItem *indexToItem(const QModelIndex &index) const;
    QtBrowserItem *indexToBrowserItem(const QModelIndex &index) const;
    bool lastColumn(int column) const;
    void disableItem(QTreeWidgetItem *item) const;
    void enableItem(QTreeWidgetItem *item) const;
    bool hasValue(QTreeWidgetItem *item) const;

    void slotCollapsed(const QModelIndex &index);
    void slotExpanded(const QModelIndex &index);

    QColor calculatedBackgroundColor(QtBrowserItem *item) const;

    QtPropertyEditorView *treeWidget() const { return m_treeWidget; }
    bool markPropertiesWithoutValue() const { return m_markPropertiesWithoutValue; }

    QtBrowserItem *currentItem() const;
    void setCurrentItem(QtBrowserItem *browserItem, bool block);
    void editItem(QtBrowserItem *browserItem);

    void slotCurrentBrowserItemChanged(QtBrowserItem *item);
    void slotCurrentTreeItemChanged(QTreeWidgetItem *newItem, QTreeWidgetItem *);

    QTreeWidgetItem *editedItem() const;

private:
    void updateItem(QTreeWidgetItem *item);

    QMap<QtBrowserItem *, QTreeWidgetItem *> m_indexToItem;
    QMap<QTreeWidgetItem *, QtBrowserItem *> m_itemToIndex;

    QMap<QtBrowserItem *, QColor> m_indexToBackgroundColor;

    QtPropertyEditorView *m_treeWidget;

    bool m_headerVisible;
    QtTreePropertyBrowser::ResizeMode m_resizeMode;
    class QtPropertyEditorDelegate *m_delegate;
    bool m_markPropertiesWithoutValue;
    bool m_browserChangedBlocked;
    QIcon m_expandIcon;
};

// ------------ QtPropertyEditorView
class QtPropertyEditorView : public QTreeWidget
{
    Q_OBJECT
public:
    QtPropertyEditorView(QWidget *parent = 0);

    void setEditorPrivate(QtTreePropertyBrowserPrivate *editorPrivate)
        { m_editorPrivate = editorPrivate; }

    QTreeWidgetItem *indexToItem(const QModelIndex &index) const
        { return itemFromIndex(index); }

protected:
    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    QtTreePropertyBrowserPrivate *m_editorPrivate;
};


// ------------ QtPropertyEditorDelegate
class QtPropertyEditorDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    QtPropertyEditorDelegate(QObject *parent = 0)
        : QItemDelegate(parent), m_editorPrivate(0), m_editedItem(0), m_editedWidget(0), m_disablePainting(false)
        {}

    void setEditorPrivate(QtTreePropertyBrowserPrivate *editorPrivate)
        { m_editorPrivate = editorPrivate; }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
            const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
            const QModelIndex &index) const;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
            const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void setModelData(QWidget *, QAbstractItemModel *,
            const QModelIndex &) const {}

    void setEditorData(QWidget *, const QModelIndex &) const {}

    bool eventFilter(QObject *object, QEvent *event);
    void closeEditor(QtProperty *property);

    QTreeWidgetItem *editedItem() const { return m_editedItem; }

protected:

    void drawDecoration(QPainter *painter, const QStyleOptionViewItem &option,
            const QRect &rect, const QPixmap &pixmap) const;
    void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option,
            const QRect &rect, const QString &text) const;

private slots:
    void slotEditorDestroyed(QObject *object);

private:
    int indentation(const QModelIndex &index) const;

    typedef QMap<QWidget *, QtProperty *> EditorToPropertyMap;
    mutable EditorToPropertyMap m_editorToProperty;

    typedef QMap<QtProperty *, QWidget *> PropertyToEditorMap;
    mutable PropertyToEditorMap m_propertyToEditor;
    QtTreePropertyBrowserPrivate *m_editorPrivate;
    mutable QTreeWidgetItem *m_editedItem;
    mutable QWidget *m_editedWidget;
    mutable bool m_disablePainting;
};


#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif
