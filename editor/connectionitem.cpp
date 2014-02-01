/*
    Copyright 2013 Jan Grulich <jgrulich@redhat.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "connectionitem.h"

#include <QtCore/QDateTime>
#include <KIcon>

ConnectionItem::ConnectionItem(QTreeWidgetItem * parent, const QStringList & strings, bool active):
    QTreeWidgetItem(parent, strings, UserType)
{
    if (active) {
        setIcon(0, KIcon("user-online"));
    } else {
        setIcon(0, KIcon("user-offline"));
    }
}

bool ConnectionItem::operator<(const QTreeWidgetItem &other) const
{
    QTreeWidget * view = treeWidget();
    const int column = view ? view->sortColumn() : 0;

    if (column == 0) { // name
        const QString a = data(column, Qt::DisplayRole).toString();
        const QString b = other.data(column, Qt::DisplayRole).toString();
        return (QString::localeAwareCompare(a, b) < 0);
    } else if (column == 1) { // last used
        const QDateTime a = data(1, ConnectionLastUsedRole).toDateTime();
        const QDateTime b = other.data(1, ConnectionLastUsedRole).toDateTime();
        return (a < b);
    }

    return QTreeWidgetItem::operator <(other);
}