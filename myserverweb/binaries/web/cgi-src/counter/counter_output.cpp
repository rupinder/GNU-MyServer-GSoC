/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "counter_output.h"
#include "numbers.h" // insert the raw image data into the binary

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "png.h"
}
#include "writepng.h"

cgi_manager * cgi_manager_ptr; // a nasty global 

// pnguser functions
void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	cgi_manager_ptr->Write(data, (int)length);
}
void user_flush_data(png_structp png_ptr)
{

}

void Counter_Output::setNumber(unsigned long int num)
{
	number = num;
}

void Counter_Output::setWrite(cgi_manager * ptr)
{
	cgi_manager_ptr = ptr;
}

void Counter_Output::run()
{
	unsigned int i;
	unsigned int digets;
	
	// how many numbers
	i = 0;
	while(number >= pow((long double)10, (long double)i))
	{
		i++;
	}
	
	digets = i;
	
	// alocate the counter image
	outBuffer = new unsigned char [(numbers_image.width / 10) * digets * numbers_image.bytes_per_pixel * numbers_image.height + 1];
	
	unsigned int num;
	unsigned long int out_offset = 0;
	unsigned long int in_offset = 0;
	unsigned int x, y;
	unsigned long int in_x, out_x, in_y_offset, out_y_offset;
	unsigned long int in_index, out_index;
	
	// This is going to get ugly!
	for(i = digets; i >= 1; i--)
	{
		num = (int)((number % (unsigned long int)pow((long double)10, (long double)i)) / pow((long double)10, (long double)(i - 1)));
		
		in_offset  = (numbers_image.width / 10) * numbers_image.bytes_per_pixel * num;
		
		out_offset = (numbers_image.width / 10) * numbers_image.bytes_per_pixel * (digets - i);
		
		for(y = 0; y < numbers_image.height; y++)
		{
			in_y_offset  = y * (numbers_image.width * numbers_image.bytes_per_pixel);
			out_y_offset = y * ((numbers_image.width / 10) * digets * numbers_image.bytes_per_pixel);
			
			for(x = 0; x < ((numbers_image.width / 10) * numbers_image.bytes_per_pixel); x++)
			{
				in_x  = x + in_offset;
				out_x = x + out_offset;
				
				in_index  = in_x  + in_y_offset;
				out_index = out_x + out_y_offset;

				outBuffer[out_index] = numbers_image.pixel_data[in_index];
			}
		}
	}
	// Take a breath, its done
	
	// write the image
	mainprog_info png_enc;
	
	png_enc.width = (numbers_image.width / 10) * digets;
	png_enc.height = numbers_image.height;
	
	png_enc.image_data = (uch*)outBuffer;
	
	// setup the row pointers for png
	unsigned char ** row_pointers= new unsigned char*[numbers_image.height];
	
	for(i = 0; i < numbers_image.height; i++)
	{
		row_pointers[i] = &outBuffer[i * (png_enc.width * numbers_image.bytes_per_pixel)];
	}
	
	png_enc.row_pointers = (uch**)row_pointers;
	
	// set the image values
	png_enc.filter = 0;
	png_enc.pnmtype = 6; //RGB
	png_enc.sample_depth = 8;
	png_enc.interlaced = 0;
	png_enc.have_bg = 0;
	png_enc.have_time = 0;
	png_enc.have_text = 0;
	
	png_enc.gamma = 0;
	
	// setup our writing function
	png_structp  *png_ptr = get_png_ptr(&png_enc);
	png_set_write_fn(*png_ptr, &png_enc, user_write_data, user_flush_data);

	writepng_init(&png_enc);
	writepng_encode_image(&png_enc);
	writepng_encode_finish(&png_enc);
	writepng_cleanup(&png_enc);
	
	delete[] row_pointers;
	delete[] outBuffer;
	
}

