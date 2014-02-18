/*
 * (C) Copyright 2002
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * tractor tw8832 test Utilities
 */

#ifndef __TW8832_H
#define __TW8832_H
#include <common.h>
#include <command.h>

#define TW8832_I2C_ADDRESS	0x45

typedef struct _tw8832_para_struct
{
	u8 tregister;
	u8 tvalue;
}tw8832_para_struct;


tw8832_para_struct const tw8832_rcamera_channel[] =
{
{0xff,0x01},
{0x02,0x40},
};


tw8832_para_struct const tw8832_pal_channel[] = {
		{0xff,0x00},
		{0x02,0x22},
		{0x07,0x01},
		{0x08,0xC6},
		{0x40,0x30},
		{0x49,0x00},
		{0x4A,0x01},
		{0xEA,0x04},
		{0xFF,0x01},
		{0x06,0x40},
		{0x07,0x12},
		{0x09,0x20},
		{0x0C,0xCC},
		{0x10,0xD8},
		{0x18,0xD0},
		//{0x1C,0x17},
		{0x1E,0x10},
		{0x2F,0xE4},
		{0x30,0x00},
		{0x33,0x45},
		{0x1C,0x17},
		{0xFF,0x02},
		{0x03,0x2E},
		{0x05,0xDF},
		{0x06,0x12},
		{0x0B,0x30},
		{0x11,0x60},
		{0x14,0x03},
		{0x18,0x4D},
		{0x83,0x80},
};
tw8832_para_struct const tw8832_ntsc_channel[] = {
		{0xff,0x00},
		{0x02,0x23},
		{0x07,0x01},
		{0x08,0xC6},
		{0x40,0x30},
		{0x49,0x83},
		{0x4A,0x01},
		{0xEA,0x14},
		{0xFF,0x01},
		{0x06,0x00},
		{0x07,0x02},
		{0x09,0xF0},
		{0x0C,0xDC},
		{0x10,0x00},
		{0x18,0x44},
		{0x1C,0x0F},
		{0x1E,0x00},
		{0x2F,0xE4},
		{0x30,0x00},
		{0x33,0x05},
		//{0x1C,0x17},
		{0xFF,0x02},
		{0x03,0x70},
		{0x05,0xDD},
		{0x06,0x0F},
		{0x0B,0x5F},
		{0x11,0x60},
		{0x14,0x05},
		{0x18,0x4E},
		{0x83,0x80},
};


tw8832_para_struct const tw8832_para_pages[] =
{                          
	{0xFF,0x00},
	{0x02,0x22},
	{0x03,0xFF},
	{0x06,0x26},
	{0x07,0x01},
	{0x08,0xC6},
	{0x09,0x00},
	{0x40,0x30},
	{0x41,0x00},
	{0x42,0x02},
	{0x43,0x10},
	{0x44,0xF0},
	{0x45,0x82},
	{0x46,0xD0},
	{0x47,0x00},
	{0x48,0x00},
	{0x49,0x00},
	{0x4A,0x01},
	{0xDB,0x25},
	{0xDC,0x00},
	{0xDD,0x80},
	{0xDE,0x00},
	{0xDF,0x80},
	{0xE0,0xF1},
	{0xE1,0x77},
	{0xE2,0x04},
	{0xE3,0x40},
	{0xE4,0x84},
	{0xE5,0x80},
	{0xE6,0x20},
	{0xE7,0x00},
	{0xE8,0xD1},
	{0xE9,0x06},
	{0xEA,0x04},
	{0xEB,0x5F},
	{0xEC,0x20},
	{0xED,0x40},
	{0xEE,0x20},
	{0xF6,0x80},
	{0xF7,0x16},
	{0xF8,0x01},
	{0xF9,0xA0},
	{0xFA,0x00},
	{0xFB,0x40},
	{0xFC,0x30},
	{0xFD,0x21},
	{0xFF,0x01},
	{0x02,0x40},
	{0x04,0x00},
	{0x05,0x0F},
	{0x06,0x40},
	{0x07,0x12},
	{0x08,0x15},
	{0x09,0x20},
	{0x0A,0x0F},
	{0x0B,0xD0},
	{0x0C,0xCC},
	{0x10,0xD8},
	{0x11,0x5c},
	{0x12,0x11},
	{0x13,0xA0},
	{0x14,0xA0},
	{0x15,0x00},
	{0x17,0x80},
	{0x18,0xD0},
	{0x1D,0x7F},
	{0x1E,0x10},
	{0x20,0x50},
	{0x21,0x22},
	{0x22,0xF0},
	{0x23,0xD8},
	{0x24,0xBC},
	{0x25,0xB8},
	{0x26,0x44},
	{0x27,0x38},
	{0x28,0x00},
	{0x29,0x00},
	{0x2A,0x78},
	{0x2B,0x44},
	{0x2C,0x30},
	{0x2D,0x14},
	{0x2E,0xA5},
	{0x2F,0xE4},
	{0x30,0x00},
	{0x33,0x45},
	{0x35,0x00},
	{0xC0,0x01},
	{0xC1,0xC7},
	{0xC2,0x01},
	{0xC3,0x03},
	{0xC4,0x5A},
	{0xC5,0x00},
	{0xC6,0x20},
	{0xC7,0x04},
	{0xC8,0x00},
	{0xC9,0x06},
	{0xCA,0x06},
	{0xCB,0x10},
	{0xCC,0x00},
	{0xCD,0x54},
	{0xD0,0x00},
	{0xD1,0xF0},
	{0xD2,0xF0},
	{0xD3,0xF0},
	{0xD4,0x00},
	{0xD5,0x00},
	{0xD6,0x10},
	{0xD7,0x70},
	{0xD8,0x00},
	{0xD9,0x04},
	{0xDA,0x80},
	{0xDB,0x80},
	{0xDC,0x20},
	{0x1C,0x17},
	{0xFF,0x02},
	{0x01,0x00},
	{0x02,0x20},
	{0x03,0x2E},
	{0x04,0x09},
	{0x05,0xDF},
	{0x06,0x12},
	{0x07,0xBF},
	{0x08,0x8F},
	{0x09,0x00},
	{0x0A,0x01},
	{0x0B,0x30},
	{0x0C,0xD0},
	{0x0D,0x90},
	{0x0E,0x00},
	{0x0F,0x02},
	{0x10,0x84},
	{0x11,0x60},
	{0x12,0x09},
	{0x13,0x5A},
	{0x14,0x03},
	{0x15,0x2A},
	{0x16,0xE0},
	{0x17,0x01},
	{0x18,0x4D},
	{0x19,0x00},
	{0x1A,0x00},
	{0x1B,0x00},
	{0x40,0x11},
	{0x41,0x0A},
	{0x42,0x05},
	{0x43,0x01},
	{0x44,0x64},
	{0x45,0xF4},
	{0x46,0x00},
	{0x47,0x0A},
	{0x48,0x36},
	{0x49,0x10},
	{0x4A,0x00},
	{0x4B,0x00},
	{0x4C,0x00},
	{0x4D,0x44},
	{0x4E,0x04},
	{0x80,0x00},
	{0x81,0x80},
	{0x82,0x80},
	{0x83,0x80},
	{0x84,0x80},
	{0x85,0x80},
	{0x86,0x80},
	{0x87,0x80},
	{0x88,0x80},
	{0x89,0x80},
	{0x8A,0x80},
	{0x8B,0x44},
	{0x8C,0x00},
	{0xB0,0x10},
	{0xB1,0x40},
	{0xB2,0x40},
	{0xB6,0x67},
	{0xB7,0x94},
	{0xBF,0x0E},
	{0xE0,0x0B},
	{0xE4,0x00},
	{0xF8,0x00},
	{0xF9,0x80},
};

void tw8832_reset();

#endif