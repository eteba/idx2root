#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include "TFile.h"
#include "TTree.h"

uint32_t read32int(FILE* pFile);

/*
 * Quick program to convert the MNIST number
 * database to .root files for educational purposes.
 * 
 * COMPILATION:
 * 		g++ idx2root.cc -o idx2root `root-config --cflags --libs`
 * or
 * 		g++ idx2root.cc -o idx2root -I/usr/lib64/root/6.14/include -L/usr/lib64/root/6.14/lib64 -lCore -lRIO -lTree
 */
int main(int argc, char **argv)
{
  if(argc != 2)
  {
	printf("Usage: ./idx2root input_idx\n");
	return 0;
  }
  
  // Read idx file in binary mode.
  FILE* pFile = fopen(argv[1], "rb");
  if(pFile == NULL)
  {
	fprintf(stderr, "Error: File could not be opened.\n");
	return 0;
  }

  // The 4 bytes at offset 0000 in the file are a
  // 32bit int magic number.
  // The following 4 bytes are the number of elements
  // in the file.
  int MagicNumber = read32int(pFile);
  int length = read32int(pFile);
  
  // Sanity check
  if( (MagicNumber != 2049 ) && (MagicNumber != 2051) )
  {
	fprintf(stderr, "Error: Not a valid MNIST idx file.\n");
	return 0;
  }
  
  printf("Magic number: %i\n", MagicNumber);
  printf("Number of elements: %i\n", length);
  
  // Open output .root file
  const char* foutName = strcat(argv[1], ".root");
  TFile* fout = new TFile(foutName, "RECREATE");
  TTree* tout = new TTree("MNIST", "MNIST_data");
    
  
  // MagicNumber == 2049 -> LABELS file.
  // MagicNumber == 2051 -> IMAGES file.
  if(MagicNumber == 2049)	// Labels
  {
	printf("Detected label file.\n");
	uint8_t labelbuf = -1;	
	int label;
	tout->Branch("labels", &label, "labels/I");
	
	// Each label is 1 byte long. No need to worry about
	// endianness.
	unsigned char* buffer = (unsigned char*)malloc(1);
	for(int n=0; n<length; n++)
	{
	  fread(buffer, 1, 1, pFile);
	  labelbuf = *((uint8_t*)buffer);
	  label = labelbuf;					// buffer points to 8 bits.
										// We need this to correctly
										// get the label number in an
										// int (~four 8 bits blocks).
	  tout->Fill();
	}
	free(buffer);
  }
  else if(MagicNumber == 2051)	// Images
  {
	printf("Detected image file.\n");
	// For images, the two 32bits offset following the
	// number of images are the number of rows and the
	// number of columns in the image.
	int nrows = read32int(pFile);
	int ncolumns = read32int(pFile);
	printf("ROWS: %i\n", nrows);
	printf("COLUMNS: %i\n", ncolumns);
	
	// I save the images in vectors.
	std::vector<uint8_t> image;
	tout->Branch("images", &image);
	
	// Loop over all the images.
	unsigned char* buffer = (unsigned char*)malloc(1);
	for(int n=0; n<length; n++)
	{
	  if(n%1000 == 0)
		printf("Image %i of %i\n", n+1, length);
	  
	  image.clear();
	  // i: Rows
	  // j: Columns
	  //    Images are stored row-wise. This decides the looping order.
	  for(int i=0; i<nrows; i++)
	  {
		for(int j=0; j<ncolumns; j++)
		{
		  fread(buffer, 1, 1, pFile);
		  image.push_back( *((uint8_t*)buffer) );
		}
	  }
	  tout->Fill();
	}
	free(buffer);
  }
  
  fout->Write();
//  fout->Close();
  
  delete tout;
  delete fout;

  fclose(pFile);
  
  return 0;
}


/*
 * MNIST database (http://yann.lecun.com/exdb/mnist/)
 * Read 32 bits and store the results in an uint32_t.
 * Convert big endian to little endian.
 */
uint32_t read32int(FILE* pFile)
{
  // NOTE: malloc needs to be casted because it is a C++ program.
  //       In C, malloc's void* is promoted to any other type.
  unsigned char* buffer = (unsigned char*)malloc(sizeof(uint32_t));
  
  // Fill buffer with an array of 4 elements of 1 byte
  fread(buffer, 1, sizeof(uint32_t), pFile);
  
  // Convert to little endian
  unsigned char* bufferLE = (unsigned char*)malloc(sizeof(uint32_t));
  for(int i=0; i<sizeof(uint32_t); i++)
	bufferLE[i] = buffer[(sizeof(uint32_t)-1)-i];
  
  // Now, the block of memory pointed by bufferLE should be
  // a 32bit int.
  uint32_t number32 = *((int*)bufferLE);
  
  free(buffer);
  free(bufferLE);
  
  return number32;
}
