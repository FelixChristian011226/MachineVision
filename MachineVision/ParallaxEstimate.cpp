#include<iostream>
#include<opencv2/opencv.hpp>

using namespace std;
using namespace cv;



class SAD
{
private:
	int winSize;//����˳ߴ�
	int DSR;//�Ӳ�������Χ
public:
	SAD() :winSize(7), DSR(30) {}
	SAD(int _winSize, int _DSR) :winSize(_winSize), DSR(_DSR) {}
	Mat computerSAD(Mat& L, Mat& R);//����SAD
};

Mat SAD::computerSAD(Mat& L, Mat& R)
{
	int Height = L.rows;
	int Width = L.cols;
	Mat Kernel_L(Size(winSize, winSize), CV_8U, Scalar::all(0));
	//CV_8U:0~255��ֵ�������ͼ��/��Ƶ�ĸ�ʽ���ö�����ȫ0����
	Mat Kernel_R(Size(winSize, winSize), CV_8U, Scalar::all(0));
	Mat Disparity(Height, Width, CV_8U, Scalar(0));


	for (int i = 0; i < Width - winSize; ++i) {
		for (int j = 0; j < Height - winSize; ++j) {
			Kernel_L = L(Rect(i, j, winSize, winSize));//LΪ��ͼ��KernelΪ�����Χ�ڵ���ͼ
			Mat MM(1, DSR, CV_32F, Scalar(0));//����ƥ�䷶Χ

			for (int k = 0; k < DSR; ++k) {
				int x = i - k;
				if (x >= 0) {
					Kernel_R = R(Rect(x, j, winSize, winSize));
					Mat Dif;
					absdiff(Kernel_L, Kernel_R, Dif);
					Scalar ADD = sum(Dif);
					float a = ADD[0];
					MM.at<float>(k) = a;
				}
				Point minLoc;
				minMaxLoc(MM, NULL, NULL, &minLoc, NULL);

				int loc = minLoc.x;
				Disparity.at<char>(j, i) = loc * 16;
			}
			double rate = double(i) / (Width);
			cout << "�����" << setprecision(2) << rate * 100 << "%" << endl;
		}
	}
	return Disparity;
}

class SGBM
{
private:
	enum mode_view { LEFT, RIGHT };
	mode_view view;	//������Ӳ�ͼor���Ӳ�ͼ

public:
	SGBM() {};
	SGBM(mode_view _mode_view) :view(_mode_view) {};
	~SGBM() {};
	Mat computersgbm(Mat& L, Mat& R);	//����SGBM
};

Mat SGBM::computersgbm(Mat& L, Mat& R)
/*SGBM_matching SGBM�㷨
*@param Mat &left_image :��ͼ��
*@param Mat &right_image:��ͼ��
*/
{
	Mat disp;

	int numberOfDisparities = ((L.size().width / 8) + 15) & -16;
	Ptr<StereoSGBM> sgbm = StereoSGBM::create(0, 16, 3);
	sgbm->setPreFilterCap(32);

	int SADWindowSize = 5;
	int sgbmWinSize = SADWindowSize > 0 ? SADWindowSize : 3;
	sgbm->setBlockSize(sgbmWinSize);
	int cn = L.channels();

	sgbm->setP1(8 * cn * sgbmWinSize * sgbmWinSize);
	sgbm->setP2(32 * cn * sgbmWinSize * sgbmWinSize);
	sgbm->setMinDisparity(0);
	sgbm->setNumDisparities(numberOfDisparities);
	sgbm->setUniquenessRatio(10);
	sgbm->setSpeckleWindowSize(100);
	sgbm->setSpeckleRange(32);
	sgbm->setDisp12MaxDiff(1);


	Mat left_gray, right_gray;
	cvtColor(L, left_gray, COLOR_RGB2GRAY);
	cvtColor(R, right_gray, COLOR_RGB2GRAY);

	view = LEFT;
	if (view == LEFT)	//�������Ӳ�ͼ
	{
		sgbm->compute(left_gray, right_gray, disp);

		disp.convertTo(disp, CV_32F, 1.0 / 16);			//����16�õ���ʵ�Ӳ�ֵ

		Mat disp8U = Mat(disp.rows, disp.cols, CV_8UC1);
		normalize(disp, disp8U, 0, 255, NORM_MINMAX, CV_8UC1);
		imwrite("results/SGBM.jpg", disp8U);

		return disp8U;
	}
	else if (view == RIGHT)	//�������Ӳ�ͼ
	{
		sgbm->setMinDisparity(-numberOfDisparities);
		sgbm->setNumDisparities(numberOfDisparities);
		sgbm->compute(left_gray, right_gray, disp);

		disp.convertTo(disp, CV_32F, 1.0 / 16);			//����16�õ���ʵ�Ӳ�ֵ

		Mat disp8U = Mat(disp.rows, disp.cols, CV_8UC1);
		normalize(disp, disp8U, 0, 255, NORM_MINMAX, CV_8UC1);
		imwrite("results/SGBM.jpg", disp8U);

		return disp8U;
	}
	else
	{
		return Mat();
	}
}


int main()
{
	Mat left = imread("./tsukuba/tsukuba_l.png");
	Mat right = imread("./tsukuba/tsukuba_r.png");
	//-------ͼ����ʾ-----------
	namedWindow("leftimag");
	imshow("leftimag", left);

	namedWindow("rightimag");
	imshow("rightimag", right);
	//--------��SAD��ȡ�Ӳ�ͼ-----
	Mat Disparity;

	SGBM mySGBM;
	Disparity = mySGBM.computersgbm(left, right);
	/*SAD mySad(7, 30);
	Disparity = mySad.computerSAD(left, right);*/


	//-------�����ʾ------
	namedWindow("Disparity");
	imshow("Disparity", Disparity);
	//-------��β------
	waitKey(0);
	return 0;
}