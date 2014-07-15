#include "lcd.h"
#include "cpu.h"
#include "interrupt.h"
#include "sdl.h"
#include "mem.h"

static int lcd_line;

static int lcd_enabled;
static int window_tilemap_select;
static int tiledata_select;
static int window_enabled;
static int tilemap_select;
static int bg_tiledata_select;
static int sprite_size;
static int sprites_enabled;
static int bg_enabled;

struct sprite {
	int y, x, tile, flags;
};

int lcd_get_line(void)
{
	return lcd_line;
}

void lcd_write_control(unsigned char c)
{
	bg_enabled            = !!(c & 0x01);
	sprites_enabled       = !!(c & 0x02);
	sprite_size           = !!(c & 0x04);
	tilemap_select        = !!(c & 0x08);
	bg_tiledata_select    = !!(c & 0x10);
	window_enabled        = !!(c & 0x20);
	window_tilemap_select = !!(c & 0x40);
	lcd_enabled           = !!(c & 0x80);
}

static void swap(struct sprite *a, struct sprite *b)
{
	struct sprite c;

	 c = *a;
	*a = *b;
	*b =  c;
}

static void sort_sprites(struct sprite *s, int n)
{
	int swapped, i;

	do
	{
		swapped = 0;
		for(i = 0; i < n-1; i++)
		{
			if(s[i].x < s[i+1].x)
			{
				swap(&s[i], &s[i+1]);
				swapped = 1;
			}
		}
	}
	while(swapped);
}

static void render_line(int line)
{

	int i, c = 0, x;
	unsigned long colours[4] = {0xFFFFFF, 0xC0C0C0, 0x808080, 0x000000};
	struct sprite s[10];
	unsigned int *b = sdl_get_framebuffer();

	for(i = 0; i<40; i++)
	{
		int y;

		y = mem_get_raw(0xFE00 + (i*4)) - 16;
		if(line < y || line >= y+8)
			continue;

		s[c].y     = y;
		s[c].x     = mem_get_raw(0xFE00 + (i*4) + 1)-8;
		s[c].tile  = mem_get_raw(0xFE00 + (i*4) + 2);
		s[c].flags = mem_get_raw(0xFE00 + (i*4) + 3);
		c++;

		if(c == 10)
			break;
	}

	if(c)
		sort_sprites(s, c);

	for(x = 0; x < 160; x++)
	{
		unsigned int map_offset, tile_num, tile_addr;
		unsigned char b1, b2, mask, colour;

		map_offset = (line/8)*32 + x/8;
		tile_num = mem_get_raw(0x9800 + tilemap_select*0x400 + map_offset);
		if(bg_tiledata_select)
			tile_addr = 0x8000 + tile_num*16;
		else
			tile_addr = 0x9000 + ((signed char)tile_num)*16;

		b1 = mem_get_raw(tile_addr+(line%8)*2);
		b2 = mem_get_raw(tile_addr+(line%8)*2+1);
		mask = 1<<(7-(x%8));
		colour = (!!(b1&mask)<<1) | !!(b2&mask);
		b[line*640 + x] = colours[colour];
	}

	for(i = 0; i<c; i++)
	{
		for(x = s[i].x; x < s[i].x+8; x++)
		{
			unsigned int tile_addr, sprite_line, colour;
			unsigned char b1, b2, bit;
			if(x < 0)
				continue;
			sprite_line = line - s[i].y;
			tile_addr = 0x8000 + (s[i].tile*16) + sprite_line*2;
			b1 = mem_get_raw(tile_addr);
			b2 = mem_get_raw(tile_addr+1);
			bit = 1<<(x%8);
			colour = (!!(b1&bit))<<1 | !!(b2&bit);
			if(colour != 0)
				b[line*640+x] = colours[colour];
		}
	}
}

static void draw_stuff(void)
{
	unsigned int *b = sdl_get_framebuffer();
	int x, y, tx, ty;
	unsigned int colours[4] = {0xFFFFFF, 0xC0C0C0, 0x808080, 0x0};
/*
	for(ty = 0; ty < 18; ty++)
	{
	for(tx = 0; tx < 20; tx++)
	{
		unsigned int tile_num, tileaddr;

		tile_num = mem_get_raw(0x9800 + tilemap_select*0x400 + ty*32+tx);
		if(!bg_tiledata_select)
			tileaddr = 0x9000 + ((signed char)tile_num)*16;
		else
			tileaddr = 0x8000 + tile_num*16;

	for(y = 0; y<8; y++)
	{

		unsigned char b1, b2;

		b1 = mem_get_raw(tileaddr+y*2);
		b2 = mem_get_raw(tileaddr+y*2+1);
		b[(ty*640*8)+(tx*8) + (y*640) + 0] = colours[(!!(b1&0x80))<<1 | !!(b2&0x80)];
		b[(ty*640*8)+(tx*8) + (y*640) + 1] = colours[(!!(b1&0x40))<<1 | !!(b2&0x40)];
		b[(ty*640*8)+(tx*8) + (y*640) + 2] = colours[(!!(b1&0x20))<<1 | !!(b2&0x20)];
		b[(ty*640*8)+(tx*8) + (y*640) + 3] = colours[(!!(b1&0x10))<<1 | !!(b2&0x10)];
		b[(ty*640*8)+(tx*8) + (y*640) + 4] = colours[(!!(b1&0x8))<<1 | !!(b2&0x8)];
		b[(ty*640*8)+(tx*8) + (y*640) + 5] = colours[(!!(b1&0x4))<<1 | !!(b2&0x4)];
		b[(ty*640*8)+(tx*8) + (y*640) + 6] = colours[(!!(b1&0x2))<<1 | !!(b2&0x2)];
		b[(ty*640*8)+(tx*8) + (y*640) + 7] = colours[(!!(b1&0x1))<<1 | !!(b2&0x1)];
	}
	}
	}
*/

	for(ty = 0; ty < 24; ty++)
	{
	for(tx = 0; tx < 16; tx++)
	{
	for(y = 0; y<8; y++)
	{
		unsigned char b1, b2;
		int tileaddr = 0x8000 +  ty*0x100 + tx*16 + y*2;

		b1 = mem_get_raw(tileaddr);
		b2 = mem_get_raw(tileaddr+1);
		b[(ty*640*8)+(tx*8) + (y*640) + 0 + 0x1F400] = colours[(!!(b1&0x80))<<1 | !!(b2&0x80)];
		b[(ty*640*8)+(tx*8) + (y*640) + 1 + 0x1F400] = colours[(!!(b1&0x40))<<1 | !!(b2&0x40)];
		b[(ty*640*8)+(tx*8) + (y*640) + 2 + 0x1F400] = colours[(!!(b1&0x20))<<1 | !!(b2&0x20)];
		b[(ty*640*8)+(tx*8) + (y*640) + 3 + 0x1F400] = colours[(!!(b1&0x10))<<1 | !!(b2&0x10)];
		b[(ty*640*8)+(tx*8) + (y*640) + 4 + 0x1F400] = colours[(!!(b1&0x8))<<1 | !!(b2&0x8)];
		b[(ty*640*8)+(tx*8) + (y*640) + 5 + 0x1F400] = colours[(!!(b1&0x4))<<1 | !!(b2&0x4)];
		b[(ty*640*8)+(tx*8) + (y*640) + 6 + 0x1F400] = colours[(!!(b1&0x2))<<1 | !!(b2&0x2)];
		b[(ty*640*8)+(tx*8) + (y*640) + 7 + 0x1F400] = colours[(!!(b1&0x1))<<1 | !!(b2&0x1)];
	}
	}
	}
}

int lcd_cycle(void)
{
	int cycles = cpu_get_cycles();
	int this_frame;
	static int prev_line;

	if(sdl_update())
		return 0;

	this_frame = cycles % (70224/4);

	lcd_line = this_frame / (456/4);
	if(lcd_line != prev_line && lcd_line < 144)
		render_line(lcd_line);


	if(prev_line == 143 && lcd_line == 144)
	{
		draw_stuff();
		interrupt(INTR_VBLANK);
		sdl_frame();
	}
	prev_line = lcd_line;
	return 1;
}
