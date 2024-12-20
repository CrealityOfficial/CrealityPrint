#include "imageproc.h"

#include <string.h>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <queue>
#include<memory>
#include<stdio.h>
#include<algorithm>

#define HOLE_BORDER 1
#define OUTER_BORDER 2


IMG2DLib::IMG2DLib()
{
	
}
IMG2DLib::~IMG2DLib()
{}

//Saves the altered image 2D vector to a txt file. Used for debugging
void saveTextFile(string file_name, vector<vector<int>> image) {
	ofstream myfile;
	myfile.open(file_name);
	for (unsigned int row = 0; row < image.size(); ++row) {
		for (unsigned int col = 0; col < image[0].size(); ++col) {
			myfile << setw(4) << image[row][col];
		}
		myfile << endl;
	}
	myfile.close();
	return;
}

//Given a vector of vectors of contours, draw out the contour specified by seq_num
//contours[n-2] contains the nth contour since the first contour starts at 2
void drawContour(vector<vector<IMG2DLib::Point>> contours, vector<vector<Pixel>> &color, int seq_num, Pixel pix) {
	int index = seq_num - 2;
	int r, c;
	for (unsigned int i = 0; i < contours[index].size(); i++) {
		r = contours[index][i].y;
		c = contours[index][i].x;
		color[r][c] = pix;
	}
}

unsigned char saturate_uchar(int val)
{
	if (val > 255) return 255;
	else if (val < 0) return 0;

	return val;
}

void setBGR(IMG2DLib::Mat& img, int idx, int channels, unsigned char value)
{
	int pixelLen = channels > 3 ? 3 : channels;
	for (int i = 0; i < pixelLen; i++)
	{
		img.imgdata[idx * channels + i] = value;
	}
	if(channels > 3 && value > 0)
		img.imgdata[idx * channels + 3] = 255;
}

void setBGRoffset(IMG2DLib::Mat& img, int idx, int channels, unsigned char offset)
{
	int pixelLen = channels > 3 ? 3 : channels;
	for (int i = 0; i < pixelLen; i++)
	{
		img.imgdata[idx * channels + i] += offset;
	}
	if (channels > 3 && offset > 0)
		img.imgdata[idx * channels + 3] = 255;
}

Pixel getPixel(IMG2DLib::Mat& img, int channels, int idx) {
	Pixel result;
	result.blue = img.imgdata[idx * channels];
	result.green = img.imgdata[idx * channels + 1];
	result.red = img.imgdata[idx * channels + 2];
	result.alpha = img.imgdata[idx * channels + 3];
	return result;
}

void setPixel(IMG2DLib::Mat& img, int channels, int idx, Pixel pixelData) {
	img.imgdata[idx * channels] = pixelData.blue;
	img.imgdata[idx * channels + 1] = pixelData.green;
	img.imgdata[idx * channels + 2] = pixelData.red;
	img.imgdata[idx * channels + 3] = pixelData.alpha;
}

//chooses color for a contour based on its seq_num
Pixel chooseColor(int n) {
	switch (n%6) {
		case 0:
			return Pixel(255,0,0);
		case 1:
			return Pixel(255, 127, 0);
		case 2:
			return Pixel(255, 255, 0);
		case 3:
			return Pixel(0, 255, 0);
		case 4:
			return Pixel(0, 0, 255);
		case 5:
			return Pixel(139, 0, 255);
	}
}

//creates a 2D array of struct Pixel, which is the 3 channel image needed to convert the 2D vector contours to a drawn bmp file
//uses DFS to step through the hierarchy tree, can be set to draw only the top 2 levels of contours, for example.
vector<vector<Pixel>> createChannels(int h, int w, vector<Node> hierarchy, vector<vector<IMG2DLib::Point>> contours) {
	queue<int> myQueue;
	vector<vector<Pixel>> color(h,vector<Pixel> (w, Pixel((unsigned char) 0, (unsigned char)0, (unsigned char)0)));
	int seq_num;
	for (int n = hierarchy[0].first_child; n != -1; n = hierarchy[n - 1].next_sibling) {
		myQueue.push(n);
	}

	while (!myQueue.empty()) {
		seq_num = myQueue.front();
		drawContour(contours, color, seq_num, chooseColor(seq_num));
		myQueue.pop();
		for (int n = hierarchy[seq_num - 1].first_child; n != -1; n = hierarchy[n - 1].next_sibling) {
			myQueue.push(n);
		}
	}
	return color;
}

//save image to bmp
void saveImageFile(const char * file_name, int h, int w, vector<Node> hierarchy, vector<vector<IMG2DLib::Point>> contours) {
	FILE *f;

	vector<vector<Pixel>> color = createChannels(h,w,hierarchy,contours);
	unsigned char *img = NULL;
	int filesize = 54 + 3 * w*h;  //w is your image width, h is image height, both int

	img = (unsigned char *)malloc(3 * w*h);
	memset(img, 0, 3 * w*h);
	int x, y;
	unsigned char r, g, b;
	for (int i = 0; i<h; i++){
		for (int j = 0; j<w; j++){
			y = (h - 1) - i; x = j;
			r = color[i][j].red;
			g = color[i][j].green;
			b = color[i][j].blue;
			/*	        if (r > 255) r=255;
			if (g > 255) g=255;
			if (b > 255) b=255;*/
			img[(x + y * w) * 3 + 2] = (unsigned char)(r);
			img[(x + y * w) * 3 + 1] = (unsigned char)(g);
			img[(x + y * w) * 3 + 0] = (unsigned char)(b);
		}
	}

	unsigned char bmpfileheader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
	unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0 };
	unsigned char bmppad[3] = { 0,0,0 };

	bmpfileheader[2] = (unsigned char)(filesize);
	bmpfileheader[3] = (unsigned char)(filesize >> 8);
	bmpfileheader[4] = (unsigned char)(filesize >> 16);
	bmpfileheader[5] = (unsigned char)(filesize >> 24);

	bmpinfoheader[4] = (unsigned char)(w);
	bmpinfoheader[5] = (unsigned char)(w >> 8);
	bmpinfoheader[6] = (unsigned char)(w >> 16);
	bmpinfoheader[7] = (unsigned char)(w >> 24);
	bmpinfoheader[8] = (unsigned char)(h);
	bmpinfoheader[9] = (unsigned char)(h >> 8);
	bmpinfoheader[10] = (unsigned char)(h >> 16);
	bmpinfoheader[11] = (unsigned char)(h >> 24);

	f = fopen(file_name, "wb");
	fwrite(bmpfileheader, 1, 14, f);
	fwrite(bmpinfoheader, 1, 40, f);
	for (int i = 0; i<h; i++){
		fwrite(img + (w*(i) * 3), 3, w, f);
		fwrite(bmppad, 1, (4 - (w * 3) % 4) % 4, f);
	}

	free(img);
	fclose(f);
}

//step around a pixel CCW
void stepCCW(IMG2DLib::Point &current, IMG2DLib::Point pivot) {
	if (current.x > pivot.x)
		current.setPoint(pivot.y - 1, pivot.x);
	else if (current.x < pivot.x)
		current.setPoint(pivot.y + 1, pivot.x);
	else if (current.y > pivot.y)
		current.setPoint(pivot.y, pivot.x + 1);
	else if (current.y < pivot.y)
		current.setPoint(pivot.y, pivot.x - 1);
}

//step around a pixel CW
void stepCW(IMG2DLib::Point &current, IMG2DLib::Point pivot) {
	if (current.x > pivot.x)
		current.setPoint(pivot.y + 1, pivot.x);
	else if (current.x < pivot.x)
		current.setPoint(pivot.y - 1, pivot.x);
	else if (current.y > pivot.y)
		current.setPoint(pivot.y, pivot.x - 1);
	else if (current.y < pivot.y)
		current.setPoint(pivot.y, pivot.x + 1);
}

//step around a pixel CCW in the 8-connect neighborhood.
void stepCCW8(IMG2DLib::Point &current, IMG2DLib::Point pivot) {
	if (current.y == pivot.y && current.x > pivot.x)
		current.setPoint(pivot.y - 1, pivot.x + 1);
	else if (current.x > pivot.x && current.y < pivot.y)
		current.setPoint(pivot.y - 1, pivot.x);
	else if (current.y < pivot.y && current.x == pivot.x)
		current.setPoint(pivot.y - 1, pivot.x - 1);
	else if (current.y < pivot.y && current.x < pivot.x)
		current.setPoint(pivot.y, pivot.x - 1);
	else if (current.y == pivot.y && current.x < pivot.x)
		current.setPoint(pivot.y + 1, pivot.x - 1);
	else if (current.y > pivot.y && current.x < pivot.x)
		current.setPoint(pivot.y + 1, pivot.x);
	else if (current.y > pivot.y && current.x == pivot.x)
		current.setPoint(pivot.y + 1, pivot.x + 1);
	else if (current.y > pivot.y && current.x > pivot.x)
		current.setPoint(pivot.y, pivot.x + 1);
}

//step around a pixel CW in the 8-connect neighborhood.
void stepCW8(IMG2DLib::Point &current, IMG2DLib::Point pivot) {
	if (current.y == pivot.y && current.x > pivot.x)
		current.setPoint(pivot.y + 1, pivot.x + 1);
	else if (current.x > pivot.x && current.y < pivot.y)
		current.setPoint(pivot.y, pivot.x + 1);
	else if (current.y < pivot.y && current.x == pivot.x)
		current.setPoint(pivot.y - 1, pivot.x + 1);
	else if (current.y < pivot.y && current.x < pivot.x)
		current.setPoint(pivot.y - 1, pivot.x);
	else if (current.y == pivot.y && current.x < pivot.x)
		current.setPoint(pivot.y - 1, pivot.x - 1);
	else if (current.y > pivot.y && current.x < pivot.x)
		current.setPoint(pivot.y, pivot.x - 1);
	else if (current.y > pivot.y && current.x == pivot.x)
		current.setPoint(pivot.y + 1, pivot.x - 1);
	else if (current.y > pivot.y && current.x > pivot.x)
		current.setPoint(pivot.y + 1, pivot.x);
}

//checks if a given pixel is out of bounds of the image
bool pixelOutOfBounds(IMG2DLib::Point p, int numrows, int numcols) {
	return (p.x >= numcols || p.y >= numrows || p.x < 0 || p.y < 0);
}

//marks a pixel as examined after passing through
void markExamined(IMG2DLib::Point mark, IMG2DLib::Point center, bool checked[4]) {
	//p3.row, p3.col + 1
	int loc = -1;
	//    3
	//  2 x 0
	//    1
	if (mark.x > center.x)
		loc = 0;
	else if (mark.x < center.x)
		loc = 2;
	else if (mark.y > center.y)
		loc = 1;
	else if (mark.y < center.y)
		loc = 3;

	if (loc == -1)
		throw ("Error: markExamined Failed");

	checked[loc] = true;
	return;
}

//marks a pixel as examined after passing through in the 8-connected case
void markExamined8(IMG2DLib::Point mark, IMG2DLib::Point center, bool checked[8]) {
	//p3.row, p3.col + 1
	int loc = -1;
	//  5 6 7
	//  4 x 0
	//  3 2 1
	if (mark.y == center.y && mark.x > center.x)
		loc = 0;
	else if (mark.x > center.x && mark.y < center.y)
		loc = 7;
	else if (mark.y < center.y && mark.x == center.x)
		loc = 6;
	else if (mark.y < center.y && mark.x < center.x)
		loc = 5;
	else if (mark.y == center.y && mark.x < center.x)
		loc = 4;
	else if (mark.y > center.y && mark.x < center.x)
		loc = 3;
	else if (mark.y > center.y && mark.x == center.x)
		loc = 2;
	else if (mark.y > center.y && mark.x > center.x)
		loc = 1;
	if (loc == -1)
		throw ("Error: markExamined Failed");
	checked[loc] = true;
	return;
}
//checks if given pixel has already been examined
bool isExamined(bool checked[4]) {
	//p3.row, p3.col + 1
	return checked[0];
}

bool isExamined8(bool checked[8]) {
	//p3.row, p3.col + 1
	return checked[0];
}

//prints image in console
void printImage(vector<vector<int>> image, int numrows, int numcols) {
	// Now print the array to see the result
	for (int row = 0; row < numrows; ++row) {
		for (int col = 0; col < numcols; ++col) {
			cout << setw(3) << image[row][col];
		}
		cout << endl;
	}
	cout << endl;
}

//follows a border from start to finish given a starting IMG2DLib::Point
void followBorder(vector<vector<int>> &image, int row, int col, IMG2DLib::Point p2, Border NBD, vector<vector<IMG2DLib::Point>> &contours) {
	int numrows = image.size();
	int numcols = image[0].size();
	IMG2DLib::Point current(p2.y, p2.x);
	IMG2DLib::Point start(row, col);
	vector<IMG2DLib::Point> point_storage;

	//(3.1)
	//Starting from (i2, j2), look around clockwise the pixels in the neighborhood of (i, j) and find a nonzero pixel.
	//Let (i1, j1) be the first found nonzero pixel. If no nonzero pixel is found, assign -NBD to fij and go to (4).
	do {
		stepCW(current, start);
		if (current.samePoint(p2)) {
			image[start.y][start.x] = -NBD.seq_num;
			point_storage.push_back(start);
			contours.push_back(point_storage);
			return;
		}
	} while (pixelOutOfBounds(current, numrows, numcols) || image[current.y][current.x] == 0);
	IMG2DLib::Point p1 = current;
	
	//(3.2)
	//(i2, j2) <- (i1, j1) and (i3, j3) <- (i, j).
	
	IMG2DLib::Point p3 = start;
	IMG2DLib::Point p4;
	p2 = p1;
	bool checked[4];
//	bool checked[8];
	while (true) {
		//(3.3)
		//Starting from the next element of the pixel(i2, j2) in the counterclockwise order, examine counterclockwise the pixels in the
		//neighborhood of the current pixel(i3, j3) to find a nonzero pixel and let the first one be(i4, j4).
		current = p2;

		for (int i = 0; i < 4; i++)
			checked[i] = false;

		do {
			markExamined(current, p3, checked);
			stepCCW(current, p3);
		} while (pixelOutOfBounds(current, numrows, numcols) || image[current.y][current.x] == 0);
		p4 = current;

		//Change the value fi3, j3 of the pixel(i3, j3) as follows :
		//	If the pixel(i3, j3 + 1) is a 0 - pixel examined in the substep(3.3) then fi3, j3 <- - NBD.
		//	If the pixel(i3, j3 + 1) is not a 0 - pixel examined in the substep(3.3) and fi3, j3 = 1, then fi3, j3 ←NBD.
		//	Otherwise, do not change fi3, j3.

		if ( (p3.x+1 >= numcols || image[p3.y][p3.x + 1] == 0) && isExamined(checked)) {
			image[p3.y][p3.x] = -NBD.seq_num;
		}
		else if (p3.x + 1 < numcols && image[p3.y][p3.x] == 1) {
			image[p3.y][p3.x] = NBD.seq_num;
		}

		point_storage.push_back(p3);
		//printImage(image, image.size(), image[0].size());
		//(3.5)
		//If(i4, j4) = (i, j) and (i3, j3) = (i1, j1) (coming back to the starting IMG2DLib::Point), then go to(4);
		//otherwise, (i2, j2) <- (i3, j3), (i3, j3) <- (i4, j4), and go back to(3.3).
		if (p4.samePoint(start) && p3.samePoint(p1)) {
			contours.push_back(point_storage);
			return;
		}

		p2 = p3;
		p3 = p4;
	}
}

//prints the hierarchy list
void printHierarchy(vector<Node> hierarchy) {
	for (unsigned int i = 0; i < hierarchy.size(); i++) {
		cout << setw(2) << i + 1 << ":: parent: " <<setw(3)<< hierarchy[i].parent << " first child: " << setw(3) << hierarchy[i].first_child << " next sibling: " << setw(3) << hierarchy[i].next_sibling << endl;
	}
}

/*
void printTree(TreeNode *root) {
	//DFS traversal of Tree
	if (root != NULL) {
		cout << root->border.seq_num << " ";
		for (TreeNode * n = root->first_child; n != NULL; n = n->next_sibling) {
			printTree(n);
		}
	}
}
*/

vector<vector<int>> setBinaryImgData(IMG2DLib::Mat inputImage)
{
	int imageW = inputImage.imageW;
	int imageH = inputImage.imageH;
	int channels = inputImage.channels;
	vector<vector<int>> resultImg;
	resultImg.reserve(imageH);
	for (int i = 0; i < imageH; i++)
	{
		vector<int> rowData;
		rowData.reserve(imageW);
		for (int j = 0; j < imageW; j++)
		{
			int idx = i * imageW + j;
			if (inputImage.imgdata[idx * channels])
			{
				rowData.push_back(0);
			}
			else
			{
				rowData.push_back(1);
			}
		}
		resultImg.push_back(rowData);
	}

	return resultImg;
}

//二值图中找出轮廓数据（矢量数据）
vector<vector<IMG2DLib::Point>> IMG2DLib::getContours(IMG2DLib::Mat inputImage)
{
	Border NBD, LNBD;
	//a vector of vectors to store each contour.
	//contour n will be stored in contours[n-2]
	//contour 2 will be stored in contours[0], contour 3 will be stored in contours[1], ad infinitum
	vector<vector<IMG2DLib::Point>> contours; 
	vector<vector<int>> image = setBinaryImgData(inputImage);
	int imageH = inputImage.imageH;
	int imageW = inputImage.imageW;

	if (image.empty()) {
		return contours;
	}

	LNBD.border_type = HOLE_BORDER;
	NBD.border_type = HOLE_BORDER;
	NBD.seq_num = 1;

	//hierarchy tree will be stored as an vector of nodes instead of using an actual tree since we need to access a node based on its index
	//see definition for Node
	//-1 denotes NULL
	vector<Node> hierarchy;
	Node temp_node(-1, -1, -1);
	temp_node.border = NBD;
	hierarchy.push_back(temp_node);

	IMG2DLib::Point p2;
	bool border_start_found;
	for (int r = 0; r < imageH; r++) {
		LNBD.seq_num = 1;
		LNBD.border_type = HOLE_BORDER;
		for (int c = 0; c < imageW; c++) {
			border_start_found = false;
			//Phase 1: Find border
			//If fij = 1 and fi, j-1 = 0, then decide that the pixel (i, j) is the border following starting IMG2DLib::Point
			//of an outer border, increment NBD, and (i2, j2) <- (i, j - 1).
			if ((image[r][c] == 1 && c - 1 < 0) || (image[r][c] == 1 && image[r][c - 1] == 0)) {
				NBD.border_type = OUTER_BORDER;
				NBD.seq_num += 1;
				p2.setPoint(r,c-1);
				border_start_found = true;
			}

			//Else if fij >= 1 and fi,j+1 = 0, then decide that the pixel (i, j) is the border following
			//starting IMG2DLib::Point of a hole border, increment NBD, (i2, j2) ←(i, j + 1), and LNBD ← fij in case fij > 1.
			else if ( c+1 < imageW && (image[r][c] >= 1 && image[r][c + 1] == 0)) {
				NBD.border_type = HOLE_BORDER;
				NBD.seq_num += 1;
				if (image[r][c] > 1) {
					LNBD.seq_num = image[r][c];
					LNBD.border_type = hierarchy[LNBD.seq_num-1].border.border_type;
				}
				p2.setPoint(r, c + 1);
				border_start_found = true;
			}

			if (border_start_found) {
				//Phase 2: Store Parent

//				current = new TreeNode(NBD);
				temp_node.reset();
				if (NBD.border_type == LNBD.border_type) {
					temp_node.parent = hierarchy[LNBD.seq_num - 1].parent;
					temp_node.next_sibling = hierarchy[temp_node.parent - 1].first_child;
					hierarchy[temp_node.parent - 1].first_child = NBD.seq_num;
					temp_node.border = NBD;
					hierarchy.push_back(temp_node);

//					cout << "indirect: " << NBD.seq_num << "  parent: " << LNBD.seq_num <<endl;
				}
				else {
					if (hierarchy[LNBD.seq_num-1].first_child != -1) {
						temp_node.next_sibling = hierarchy[LNBD.seq_num-1].first_child;
					}

					temp_node.parent = LNBD.seq_num;
					hierarchy[LNBD.seq_num-1].first_child = NBD.seq_num;
					temp_node.border = NBD;
					hierarchy.push_back(temp_node);

//					cout << "direct: " << NBD.seq_num << "  parent: " << LNBD.seq_num << endl;
				}

				//Phase 3: Follow border
				followBorder(image, r, c, p2, NBD, contours);
			}

			//Phase 4: Continue to next border
			//If fij != 1, then LNBD <- abs( fij ) and resume the raster scan from the pixel(i, j + 1).
			//The algorithm terminates when the scan reaches the lower right corner of the picture.
			if (abs(image[r][c]) > 1) {
				LNBD.seq_num = abs(image[r][c]);
				LNBD.border_type = hierarchy[LNBD.seq_num - 1].border.border_type;
			}
		}

	}
	return contours;
}

//彩色图灰度化
void IMG2DLib::colorToGray(IMG2DLib::Mat& colorImage)
{
	int imgW = colorImage.imageW;
	int imgH = colorImage.imageH;
	int channels = colorImage.channels;

	for (int i = 0; i < imgH; i++)
	{
		for (int j = 0; j < imgW; j++)
		{
			int idx = i * imgW + j;
			unsigned char gray;
			if (channels == 1) gray = colorImage.imgdata[idx];
			else gray = 0.30 * colorImage.imgdata[idx * channels + 0] + 0.59 * colorImage.imgdata[idx * channels + 1] + 0.11 * colorImage.imgdata[idx * channels + 2];
			setBGR(colorImage, idx, channels, gray);
		}
	}
}
//彩色图灰度化，contrastValue对比度值（0~100），brightValue亮度值（0~100），setWhiteVal白色化阈值（0~255，> setWhiteVald的像素点设为255）
void IMG2DLib::colorToGray(IMG2DLib::Mat& colorImage, int contrastValue, int brightValue, int setWhiteVal)
{
	int imgW = colorImage.imageW;
	int imgH = colorImage.imageH;
	int channels = colorImage.channels;
	double contrastValueRatio = contrastValue * 0.02;
	int _brightValue = (brightValue - 50) * 0.02 * 255;

	for (int i = 0; i < imgH; i++)
	{
		for (int j = 0; j < imgW; j++)
		{
			int idx = i * imgW + j;
			unsigned char val;
			if (channels == 1) val = colorImage.imgdata[idx];
			else val = 0.30 * colorImage.imgdata[idx * channels + 0] + 0.59 * colorImage.imgdata[idx * channels + 1] + 0.11 * colorImage.imgdata[idx * channels + 2];
			val = saturate_uchar(contrastValueRatio * val + _brightValue);
			if (val > setWhiteVal) val = 255;
			setBGR(colorImage, idx, channels, val);
		}
	}
}
//灰度图转二值图
void IMG2DLib::GrayToBinary(IMG2DLib::Mat& grayImage, int value)
{
	int imgW = grayImage.imageW;
	int imgH = grayImage.imageH;
	int channels = grayImage.channels;

	for (int i = 0; i < imgH; i++)
	{
		for (int j = 0; j < imgW; j++)
		{
			int idx = i * imgW + j;
			unsigned char val = grayImage.imgdata[idx * channels] > value ? 255 : 0;
			setBGR(grayImage, idx, channels, val);
		}
	}
}
//上下翻转
void IMG2DLib::flipUpDown(IMG2DLib::Mat& image)
{
	int imgW = image.imageW;
	int imgH = image.imageH;
	int channels = image.channels;
	int halfHeight = imgH / 2;

	for (int i = 0; i < halfHeight; i++)
	{
		for (int j = 0; j < imgW; j++)
		{
			int idx_1 = i * imgW + j;
			int idx_2 = (imgH - i - 1) * imgW + j;
			if (channels == 1)
			{
				unsigned char val_1 = image.imgdata[idx_1];
				unsigned char val_2 = image.imgdata[idx_2];
				setBGR(image, idx_1, channels, val_2);
				setBGR(image, idx_2, channels, val_1);
			}
			else
			{
				Pixel p_1 = getPixel(image, idx_1, channels);
				Pixel p_2 = getPixel(image, idx_2, channels);
				setPixel(image, idx_1, channels, p_2);
				setPixel(image, idx_2, channels, p_1);
			}
		}
	}
}
//左右翻转
void IMG2DLib::flipRightLeft(IMG2DLib::Mat& image)
{
	int imgW = image.imageW;
	int imgH = image.imageH;
	int channels = image.channels;
	int halfWidth = imgW / 2;

	for (int i = 0; i < imgH; i++)
	{
		for (int j = 0; j < halfWidth; j++)
		{
			int idx_1 = i * imgW + j;
			int idx_2 = i * imgW + imgW - j - 1;
			if (channels == 1)
			{
				unsigned char val_1 = image.imgdata[idx_1];
				unsigned char val_2 = image.imgdata[idx_2];
				setBGR(image, idx_1, channels, val_2);
				setBGR(image, idx_2, channels, val_1);
			}
			else
			{
				Pixel p_1 = getPixel(image, idx_1, channels);
				Pixel p_2 = getPixel(image, idx_2, channels);
				setPixel(image, idx_1, channels, p_2);
				setPixel(image, idx_2, channels, p_1);
			}
		}
	}
}
//图像像素值反转
void IMG2DLib::reversalImgae(IMG2DLib::Mat& image)
{
	int imgW = image.imageW;
	int imgH = image.imageH;
	int channels = image.channels;
	int step = imgW * channels;

	for (int i = 0; i < imgH; i++)
	{
		for (int j = 0; j < imgW; j++)
		{
			int idx = i * imgW + j;
			if (channels == 1)
			{
				unsigned char val = 255 - image.imgdata[idx];
				setBGR(image, idx, channels, val);
			}
			else
			{
				Pixel p_1 = getPixel(image, idx, channels);
				p_1.blue = 255 - p_1.blue;
				p_1.green = 255 - p_1.green;
				p_1.red = 255 - p_1.red;
				setPixel(image, idx, channels, p_1);
			}
		}
	}
}
//抖动灰度
void IMG2DLib::DitheringImage(IMG2DLib::Mat& grayImage, int value)
{
	int imgW = grayImage.imageW;
	int imgH = grayImage.imageH;
	int channels = grayImage.channels;
;
	unsigned char* pixels = grayImage.imgdata;
	int gray = 0;
	int v = 0;
	int error = 0;
	for (int y = 0; y < imgH; y++)
	{
		for (int x = 0; x < imgW; x++)
		{
			int idx = x + y * (imgW);
			//gray scale
			if (channels == 1) gray = pixels[idx];
			else gray = 0.30 * pixels[idx * channels + 0] + 0.59 * pixels[idx * channels + 1] + 0.11 * pixels[idx * channels + 2];		
			if ((x == 1) || (x == 0))
			{
				setBGR(grayImage, idx, channels, 255);
			}
			else
			{
				setBGR(grayImage, idx, channels, gray);
			}
		}
	}
	for (int y = 0; y < imgH - 2; y++)
	{
		for (int x = 0; x < imgW - 2; x++)
		{
			int idx = x + y * (imgW);
			//gray scale
			if (channels == 1) gray = grayImage.imgdata[idx];
			else gray = 0.30 * grayImage.imgdata[idx * channels + 0] + 0.59 * grayImage.imgdata[idx * channels + 1] + 0.11 * grayImage.imgdata[idx * channels + 2];
			v = (gray >= value) ? 255 : 0;
			error = (gray - v) / 8;
			setBGR(grayImage, idx, channels, v);
			setBGRoffset(grayImage, 1 + x + y * (imgW), channels, error);
			setBGRoffset(grayImage, 2 + x + y * (imgW), channels, error);
			setBGRoffset(grayImage, x - 1 + (y + 1) * (imgW), channels, error);
			setBGRoffset(grayImage, x + (y + 1) * (imgW), channels, error);
			setBGRoffset(grayImage, 1 + x + (y + 1) * (imgW), channels, error);
			setBGRoffset(grayImage, x + (y + 2) * (imgW), channels, error);
		}
	}
}


//图像均值滤波
void IMG2DLib::blurImgae(IMG2DLib::Mat& image, int size)
{
	size /= 2;
	int imgW = image.imageW;
	int imgH = image.imageH;
	int channels = image.channels;
	int pixel_size = imgW * imgH;
	IMG2DLib::Mat src_image(imgW, imgH, channels, image.imgdata);

#pragma omp parallel for
	for (int i = 0; i < imgH; i++)
	{
		for (int j = 0; j < imgW; j++)
		{
			int index = i * imgW + j;
			int total_val = 0, pt_num = 0;
			for (int m = -size; m <= size; m++)
			{
				for (int n = -size; n <= size; n++)
				{
					int idx = (i + m) * imgW + (j + n);
					if (idx >= 0 && idx < pixel_size)
					{
						total_val += src_image.imgdata[idx * channels];
						pt_num++;
					}
				}
			}
			unsigned char val = total_val / (double)pt_num + 0.5;
			setBGR(image, index, channels, val);

		}
	}
	src_image.release();
}

//图像中值滤波
void IMG2DLib::medianBlurImgae(IMG2DLib::Mat& image, int size)
{
	int rect_pixel_num = size * size;
	size /= 2;
	int imgW = image.imageW;
	int imgH = image.imageH;
	int channels = image.channels;
	int pixel_size = imgW * imgH;
	IMG2DLib::Mat src_image(imgW, imgH, channels, image.imgdata);

#pragma omp parallel for
	for (int i = 0; i < imgH; i++)
	{
		for (int j = 0; j < imgW; j++)
		{
			int index = i * imgW + j;
			int pt_num = 0;
			std::vector<unsigned char> pixel_data;
			pixel_data.reserve(rect_pixel_num);
			for (int m = -size; m <= size; m++)
			{
				for (int n = -size; n <= size; n++)
				{
					int idx = (i + m) * imgW + (j + n);
					if (idx >= 0 && idx < pixel_size)
					{
						pixel_data.push_back(src_image.imgdata[idx * channels]);
						pt_num++;
					}
				}
			}
			std::sort(pixel_data.begin(), pixel_data.end());
			unsigned char val = pixel_data[pt_num / 2.0 + 0.5];
			setBGR(image, index, channels, val);
		}
	}
	src_image.release();
}

//二值图中值滤波
void IMG2DLib::medianBlurBinaryImgae(IMG2DLib::Mat& image, int size)
{
	int rect_pixel_num = size * size;
	size /= 2;
	int imgW = image.imageW;
	int imgH = image.imageH;
	int channels = image.channels;
	int pixel_size = imgW * imgH;
	IMG2DLib::Mat src_image(imgW, imgH, channels, image.imgdata);

#pragma omp parallel for
	for (int i = 0; i < imgH; i++)
	{
		for (int j = 0; j < imgW; j++)
		{
			int index = i * imgW + j;
			int black_num = 0;
			int white_num = 0;

			for (int m = -size; m <= size; m++)
			{
				for (int n = -size; n <= size; n++)
				{
					int idx = (i + m) * imgW + (j + n);
					if (idx >= 0 && idx < pixel_size)
					{
						if (src_image.imgdata[idx * channels] == 255)
							white_num++;
						else
							black_num++;
					}
				}
			}
			
			unsigned char val = 0;
			if (white_num > black_num) val = 255;
			setBGR(image, index, channels, val);
		}
	}
	src_image.release();
}
