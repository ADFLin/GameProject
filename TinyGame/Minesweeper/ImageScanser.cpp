#include "ImageScanser.h"

#include <fstream>
bool ScanBuilder::saveSrcData( char const* path , char const* dataName )
{
	std::ofstream  fs( path );
	if ( !fs )
		return false;

	ShapeInfo info[ 128 ];
	ScanPixel pixels[ 512 ];
	int numShape = 0;
	exportData( info , numShape , pixels );

	fs << "ShapeInfo" << "g" << dataName << "ShapeIno"  << "[] = {";
	for( int i = 0 ; i < numShape ; ++i )
	{
		fs << "{" << info[i].regID << ","
			<< info[i].error       << ","
			<< info[i].idxPixel    << "} ,";

	}
	fs << "};\n";

	fs << "ShapeInfo" << "g" << dataName << "Pixel" << "[] = {";
	for ( int i = 0 ; i < info[numShape].idxPixel ; ++i )
	{
		fs << "{" << pixels[i].color << ","
			<< pixels[i].x     << ","
			<< pixels[i].y     << "} ,";
	}
	fs << "};\n";

	return true;
}
