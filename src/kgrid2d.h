/*
    This file is part of the KDE games library
    Copyright (C) 2001-02 Nicolas Hadacek (hadacek@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef __KGRID2D_H_
#define __KGRID2D_H_

#include <cmath>

#include <QPair>
#include <QVector>
#include <QTextStream>
#include <QDataStream>

namespace Grid2D
{
/**
 * This type represents coordinates on a bidimensionnal grid.
 */
typedef QPair<int, int> Coord;

/**
 * This type represents a list of @ref Coord.
 */
typedef QList<Coord> CoordList;
}

inline Grid2D::Coord operator +(const Grid2D::Coord &c1, const Grid2D::Coord &c2)
{
    return Grid2D::Coord(c1.first + c2.first, c1.second + c2.second);
}

inline Grid2D::Coord operator -(const Grid2D::Coord &c1, const Grid2D::Coord &c2)
{
    return Grid2D::Coord(c1.first - c2.first, c1.second - c2.second);
}

/**
 * @return the maximum of both coordinates.
 */
inline Grid2D::Coord maximum(const Grid2D::Coord &c1, const Grid2D::Coord &c2)
{
    return Grid2D::Coord(qMax(c1.first, c2.first), qMax(c1.second, c2.second));
}
/**
 * @return the minimum of both coordinates.
 */
inline Grid2D::Coord minimum(const Grid2D::Coord &c1, const Grid2D::Coord &c2)
{
    return Grid2D::Coord(qMin(c1.first, c2.first), qMin(c1.second, c2.second));
}

inline QTextStream &operator <<(QTextStream &s, const Grid2D::Coord &c)
{
    return s << '(' << c.second << "," << c.first << ')';
}

inline QTextStream &operator <<(QTextStream &s, const Grid2D::CoordList &list)
{
    for (Grid2D::CoordList::const_iterator i = list.constBegin(); i != list.constEnd(); ++i) {
        s << *i;
    }
    return s;
}

//-----------------------------------------------------------------------------

namespace Grid2D {

/**
 * \class Generic kgrid2d.h <KGrid2D>
 *
 * This template class represents a generic bidimensionnal grid. Each node
 * contains an element of the template type.
 */
template <class Type>
class Generic
{
public:
    /**
     * Constructor.
     */
    Generic(uint width = 0, uint height = 0) {
        resize(width, height);
    }

    virtual ~Generic() = default;

    /**
     * Resize the grid.
     */
    void resize(uint width, uint height) {
        _width = width;
        _height = height;
        _vector.resize(width * height);
    }

    /**
     * @return the width.
     */
    uint width() const {
        return _width;
    }
    /**
     * @return the height.
     */
    uint height() const {
        return _height;
    }
    /**
     * @return the number of nodes (ie width*height).
     */
    uint size() const {
        return _width * _height;
    }

    /**
     * @return the linear index for the given coordinate.
     */
    uint index(const Coord &c) const {
        return c.first + c.second * _width;
    }

    /**
     * @return the coordinate corresponding to the linear index.
     */
    Coord coord(uint index) const {
        return Coord(index % _width, index / _width);
    }

    /**
     * @return the value at the given coordinate.
     */
    const Type &at(const Coord &c) const {
        return _vector[index(c)];
    }
    /**
     * @return the value at the given coordinate.
     */
    Type &at(const Coord &c) {
        return _vector[index(c)];
    }
    /**
     * @return the value at the given coordinate.
     */
    const Type &operator [](const Coord &c) const {
        return _vector[index(c)];
    }
    /**
     * @return the value at the given coordinate.
     */
    Type &operator [](const Coord &c) {
        return _vector[index(c)];
    }

    /**
     * @return the value at the given linear index.
     */
    const Type &at(uint index) const {
        return _vector[index];
    }
    /**
     * @return the value at the given linear index.
     */
    Type &at(uint index) {
        return _vector[index];
    }
    /**
     * @return the value at the given linear index.
     */
    const Type &operator [](uint index) const {
        return _vector[index];
    }
    /**
     * @return the value at the given linear index.
     */
    Type &operator [](uint index) {
        return _vector[index];
    }

    /**
     * @return if the given coordinate is inside the grid.
     */
    bool inside(const Coord &c) const {
        return c.first >= 0 && c.first < (int)_width && c.second >= 0 && c.second < (int)_height;
    }

protected:
    uint _width;
    uint _height;
    QVector<Type> _vector;
};

template <class Type>
QDataStream &operator <<(QDataStream &s, const Grid2D::Generic<Type> &m)
{
    s << (quint32)m.width() << (quint32)m.height();
    for (uint i = 0; i < m.size(); i++) {
        s << m[i];
    }

    return s;
}

template <class Type>
QDataStream &operator >>(QDataStream &s, Grid2D::Generic<Type> &m)
{
    quint32 w, h;
    s >> w >> h;
    m.resize(w, h);
    for (uint i = 0; i < m.size(); i++) {
        s >> m[i];
    }
    return s;
}

//-----------------------------------------------------------------------------

/**
 * \class SquareBase kgrid2d.h <KGrid2D>
 *
 * kgamecanvas.hThis class contains static methods to manipulate coordinates for a
 * square bidimensionnal grid.
 */
class SquareBase
{
public:
    /**
     * Identify the eight neighbours.
     */
    enum Neighbour {
        Left = 0,
        Right,
        Up,
        Down,
        LeftUp,
        LeftDown,
        RightUp,
        RightDown,
        Nb_Neighbour
    };

    /**
     * @return the neighbour for the given coordinate.
     */
    static Coord neighbour(const Coord &c, Neighbour n) {
        switch (n) {
            case Left:      return c + Coord(-1,  0);
            case Right:     return c + Coord(1,  0);
            case Up:        return c + Coord(0, -1);
            case Down:      return c + Coord(0,  1);
            case LeftUp:    return c + Coord(-1, -1);
            case LeftDown:  return c + Coord(-1,  1);
            case RightUp:   return c + Coord(1, -1);
            case RightDown: return c + Coord(1,  1);
            case Nb_Neighbour: Q_ASSERT(false);
        }
        return c;
    }
};

/**
 * \class Square kgrid2d.h <KGrid2D>
 *
 * This template is a @ref Generic implementation for a square bidimensionnal
 * grid (@ref SquareBase).
 */
template <class T>
class Square : public Generic<T>, public SquareBase
{
public:
    /**
     * Constructor.
     */
    Square(uint width = 0, uint height = 0)
        : Generic<T>(width, height)
    { }

    /**
     * @return the neighbours of coordinate @param c
     * to the given set of coordinates
     * @param c the coordinate to use as the reference point
     * @param insideOnly only add coordinates that are inside the grid.
     * @param directOnly only add the four nearest neighbours.
     */
    CoordList neighbours(const Coord &c, bool insideOnly = true,
                         bool directOnly = false) const {
        CoordList neighbours;
        for (int i = 0; i < (directOnly ? LeftUp : Nb_Neighbour); i++) {
            Coord n = neighbour(c, (Neighbour)i);
            if (insideOnly && !Generic<T>::inside(n)) {
                continue;
            }

            neighbours.append(n);
        }
        return neighbours;
    }

    /**
     * @return the "projection" of the given coordinate on the grid edges.
     *
     * @param c the coordinate to use as the reference point
     * @param n the direction of projection.
     */
    Coord toEdge(const Coord &c, Neighbour n) const {
        switch (n) {
            case Left:      return Coord(0, c.second);
            case Right:     return Coord(Generic<T>::width() - 1, c.second);
            case Up:        return Coord(c.first, 0);
            case Down:      return Coord(c.first, Generic<T>::height() - 1);
            case LeftUp:    return Coord(0, 0);
            case LeftDown:  return Coord(0, Generic<T>::height() - 1);
            case RightUp:   return Coord(Generic<T>::width() - 1, 0);
            case RightDown: return Coord(Generic<T>::width() - 1, Generic<T>::height() - 1);
            case Nb_Neighbour: Q_ASSERT(false);
        }
        return c;
    }
};

} // namespace Grid2D

#endif
