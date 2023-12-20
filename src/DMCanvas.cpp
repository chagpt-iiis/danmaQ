﻿/*
 * This file is part of danmaQ.
 * 
 * DanmaQ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DanmaQ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtCore>
#include <QtGui/qtextdocument.h>
#include <QApplication>
#include <QScreen>
#include <QDesktopWidget>
#include <QVector>
#include <QRegExp>
#include <QDebug>

#include <cstdlib>

#include "common.hpp"


DMCanvas::DMCanvas(QScreen* screen, DMMainWindow *parent)
{
	this->setParent(parent);
	this->mainWindow = parent;
	this->screen = screen->geometry();

	// this->sx = geo.topLeft().x();
	// this->sy = geo.topLeft().y();
	// this->sw = geo.width();
	// this->sh = geo.height();
	// myDebug << sx << ", " << sy << ", " <<sw << ", " << sh;

	// QPoint windowPos = parent->mapFromGlobal(geo.topLeft());
	// this->resize(sw, sh);
	// this->move(geo.topLeft());
	// this->setGeometry(sx, sy, sw, sh);
	// this->setWindowFlags(
	// 	Qt::X11BypassWindowManagerHint
	// 	| Qt::WindowStaysOnTopHint
	// 	| Qt::ToolTip
	// 	| Qt::FramelessWindowHint
	// );
	// this->setAttribute(Qt::WA_TranslucentBackground, true);
	// this->setAttribute(Qt::WA_DeleteOnClose, true);
	// this->setAttribute(Qt::WA_Disabled, true);
	// this->setAttribute(Qt::WA_TransparentForMouseEvents, true);
	// this->setStyleSheet("background: transparent");

	this->setWindowTitle("Danmaku");

	this->init_slots();
	
	this->hide();

}

DMCanvas::DMCanvas(DMMainWindow *parent): DMCanvas(0, parent){};

void DMCanvas::init_slots()
{
	int height = this->screen.height();
	int nlines = (height - 2 * VMARGIN) / this->mainWindow->lineHeight;
	myDebug << "nlines =" << nlines << ", lineHeight =" << this->mainWindow->lineHeight;
	this->fly_slots = QVector<bool>(nlines, false);
	this->fixed_slots = QVector<bool>(nlines, false);
	this->vertical_slots = QSet<Danmaku*>();
}

QPoint DMCanvas::getGlobalPoint(QPoint p)
{
	return this->screen.topLeft() + p;
}

int DMCanvas::allocate_slot(Position position) {
	int slot = -1;
	switch (position) {
		case topScrolling:
		case topReverse:
			for (int i = 0; i < 10; i++) {
				int try_slot;
				if (i < 5) {
					try_slot = std::rand() % (this->fly_slots.size() / 2);
				} else {
					try_slot = std::rand() % (this->fly_slots.size());
				}
				if (this->fly_slots.at(try_slot) == false) {
					this->fly_slots[try_slot] = true;
					slot = try_slot;
					break;
				}
			}
			break;
		case bottomScrolling:
			for (int i = 0; i < 10; i++) {
				int try_slot;
				if (i < 5) {
					try_slot = this->fly_slots.size() / 2 + std::rand() % (this->fly_slots.size() / 2);
				} else {
					try_slot = std::rand() % (this->fly_slots.size());
				}
				if (this->fly_slots.at(try_slot) == false) {
					this->fly_slots[try_slot] = true;
					slot = try_slot;
					break;
				}
			}
			break;
		case topStatic:
			for (int i = 0; i < this->fixed_slots.size(); i++) {
				if (this->fixed_slots.at(i) == false) {
					this->fixed_slots[i] = true;
					slot = i;
					break;
				}
			}
			break;
		case bottomStatic:
			for (int i = this->fixed_slots.size() - 1; i >= 0; i--) {
				if (this->fixed_slots.at(i) == false) {
					this->fixed_slots[i] = true;
					slot = i;
					break;
				}
			}
			break;
		default:
			break;
	}
	myDebug << "Slot: " << slot;
	return slot;
}

int DMCanvas::slot_y(int slot)
{
	return (this->mainWindow->lineHeight * slot + VMARGIN);
}

void DMCanvas::new_danmaku(QString text, int color, int position)
{
	Position pos = static_cast<Position>(position);
	if (pos > 0 && pos < position_name.size())
	{
		myDebug << "position: " << position_name[position];
	}
	else
	{
		myDebug << "wrong position: " << pos;
		return;
	}

	Danmaku *l;
	if (position == vertical) {
		int slot = this->fixed_slots.size() - 1;
		l = new Danmaku(text, color, pos, slot, this, this->mainWindow);
		int dy = l->height() + 14;
		QList<Danmaku *> cur = this->vertical_slots.values();
		for (Danmaku *dm : cur) dm->shift_up(dy);
		vertical_slots.insert(l);
	} else {
		int slot = allocate_slot(pos);
		if (slot < 0) {
			myDebug << "Screen is Full!";
			return;
		}

		l = new Danmaku(text, color, pos, slot, this, this->mainWindow);
	}
	this->connect(l, &Danmaku::exited,
				  this, &DMCanvas::delete_danmaku);
	l->show();
}


void DMCanvas::delete_danmaku(Danmaku* dm) {
	switch (dm->position){
		case topScrolling:
		case bottomScrolling:
		case topReverse:
			this->fly_slots[dm->slot] = false;
			break;
		case topStatic:
		case bottomStatic:
			this->fixed_slots[dm->slot] = false;
			break;
		case vertical:
			this->vertical_slots.remove(dm);
			break;
		default:
			break;
	}
	myDebug << "danmaku closed";
}

DMCanvas::~DMCanvas() {
	myDebug << "window closed";
}
