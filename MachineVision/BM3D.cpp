#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

const int blockSize = 8;     // ���С
const int searchWindow = 16; // �������ڴ�С
const int numBlocks = 2;     // ������

// �����֮������ƶȣ�ŷ�Ͼ��룩
double blockDistance(const Mat& block1, const Mat& block2) {
    return norm(block1, block2, NORM_L2);
}

// ���п�ƥ��
void blockMatching(const Mat& noisyImage, vector<Mat>& matchedBlocks, int row, int col) {
    Mat block = noisyImage(Rect(col, row, blockSize, blockSize));

    for (int i = -searchWindow; i <= searchWindow; i++) {
        for (int j = -searchWindow; j <= searchWindow; j++) {
            int newRow = row + i;
            int newCol = col + j;
            if (newRow >= 0 && newCol >= 0 && newRow + blockSize <= noisyImage.rows && newCol + blockSize <= noisyImage.cols) {
                Mat candidateBlock = noisyImage(Rect(newCol, newRow, blockSize, blockSize));
                if (blockDistance(block, candidateBlock) < 1000) { // ���ƶ���ֵ
                    matchedBlocks.push_back(candidateBlock);
                }
            }
        }
    }
}

// 3D �˲�
Mat threeDFiltering(const vector<Mat>& blocks) {
    if (blocks.empty()) return Mat::zeros(blockSize, blockSize, CV_8UC1);

    Mat filteredBlock = Mat::zeros(blockSize, blockSize, CV_8UC1);
    for (const auto& block : blocks) {
        filteredBlock += block;
    }
    filteredBlock /= static_cast<double>(blocks.size());

    // ȷ������ֵ��0��255֮��
    filteredBlock = cv::min(cv::max(filteredBlock, 0), 255);
    return filteredBlock;
}


// BM3D ȥ���㷨
Mat bm3dDenoising(const Mat& noisyImage) {
    Mat denoisedImage = Mat::zeros(noisyImage.size(), noisyImage.type());

    for (int row = 0; row < noisyImage.rows - blockSize; row += blockSize) {
        for (int col = 0; col < noisyImage.cols - blockSize; col += blockSize) {
            vector<Mat> matchedBlocks;
            blockMatching(noisyImage, matchedBlocks, row, col);

            if (!matchedBlocks.empty()) {
                Mat filteredBlock = threeDFiltering(matchedBlocks);
                filteredBlock.copyTo(denoisedImage(Rect(col, row, blockSize, blockSize)));
            }
        }
    }

    return denoisedImage;
}

int main() {
    // ��ȡ����ͼ��
    Mat noisyImage = imread("Outputs\\noisy_image.jpg", IMREAD_GRAYSCALE);
    if (noisyImage.empty()) {
        cerr << "Failed to load image." << endl;
        return -1;
    }

    // ִ�� BM3D ȥ��
    Mat denoisedImage = bm3dDenoising(noisyImage);

    // ��ʾ���
    imshow("Noisy Image", noisyImage);
    imshow("Denoised Image", denoisedImage);
    waitKey(0);
    return 0;
}
