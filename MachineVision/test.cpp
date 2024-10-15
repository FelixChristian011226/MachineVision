#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;
using namespace cv;

void addGaussianNoise(cv::Mat& image, double mean, double sigma) {
    cv::Mat noise(image.size(), image.type());
    cv::randn(noise, mean, sigma); // ��˹����
    image += noise;
}

void gaussianBlur(cv::Mat& image, int kernelSize) {
    cv::Mat temp = image.clone();
    int radius = kernelSize / 2;
    std::vector<double> kernel(kernelSize * kernelSize);

    // Generate Gaussian kernel
    double sigma = 1.0;
    double sum = 0.0;
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            double value = exp(-(x * x + y * y) / (2 * sigma * sigma)) / (2 * M_PI * sigma * sigma);
            kernel[(y + radius) * kernelSize + (x + radius)] = value;
            sum += value;
        }
    }

    // Normalize Gaussian kernel
    for (auto& val : kernel) {
        val /= sum;
    }

    // Apply Gaussian blur
    if (image.channels() == 3) {  // For 3-channel images
        for (int y = radius; y < image.rows - radius; y++) {
            for (int x = radius; x < image.cols - radius; x++) {
                cv::Vec3d newPixel = cv::Vec3d(0, 0, 0);
                for (int ky = -radius; ky <= radius; ky++) {
                    for (int kx = -radius; kx <= radius; kx++) {
                        newPixel += kernel[(ky + radius) * kernelSize + (kx + radius)] *
                            image.at<cv::Vec3b>(y + ky, x + kx);
                    }
                }
                image.at<cv::Vec3b>(y, x) = cv::Vec3b(cv::saturate_cast<uchar>(newPixel[0]),
                    cv::saturate_cast<uchar>(newPixel[1]),
                    cv::saturate_cast<uchar>(newPixel[2]));
            }
        }
    }
    else if (image.channels() == 1) {  // For single-channel images
        for (int y = radius; y < image.rows - radius; y++) {
            for (int x = radius; x < image.cols - radius; x++) {
                double newPixel = 0.0;
                for (int ky = -radius; ky <= radius; ky++) {
                    for (int kx = -radius; kx <= radius; kx++) {
                        newPixel += kernel[(ky + radius) * kernelSize + (kx + radius)] *
                            image.at<uchar>(y + ky, x + kx);
                    }
                }
                image.at<uchar>(y, x) = cv::saturate_cast<uchar>(newPixel);
            }
        }
    }
}


//NL-means 
void nonlocalMeansFilter(Mat& src, Mat& dest, int templeteWindowSize, int searchWindowSize, double h, double sigma = 0.0)
{

    if (dest.empty())
        dest = Mat::zeros(src.size(), src.type());

    const int tr = templeteWindowSize >> 1;                 //��������λ��
    const int sr = searchWindowSize >> 1;                   //����������λ��
    const int bb = sr + tr;                                 //���ӱ߽�
    const int D = searchWindowSize * searchWindowSize;      //������Ԫ�ظ���
    const int H = D / 2 + 1;                                //���������ĵ�λ��
    const double div = 1.0 / (double)D;                     //���ȷֲ�ʱ���������е�ÿ�����Ȩ�ش�С
    const int tD = templeteWindowSize * templeteWindowSize; //�����е�Ԫ�ظ���
    const double tdiv = 1.0 / (double)(tD);                 //���ȷֲ�ʱ���������е�ÿ�����Ȩ�ش�С
    Mat im;
    copyMakeBorder(src, im, bb, bb, bb, bb, cv::BORDER_DEFAULT);
    //����Ȩ��
    vector<double> weight(256 * 256 * src.channels());
    double* w = &weight[0];
    const double gauss_sd = (sigma == 0.0) ? h : sigma;                             //��˹��׼��
    double gauss_color_coeff = -(1.0 / (double)(src.channels())) * (1.0 / (h * h)); //��˹��ɫϵ��
    int emax = 0;


    //w[i]���淽�������ƽ��ŷ�Ͼ����Ӧ�ĸ�˹��ȨȨ�أ�����������ŷʽ��������
    for (int i = 0; i < 256 * 256 * src.channels(); i++)
    {
        double v = std::exp(max(i - 2.0 * gauss_sd * gauss_sd, 0.0) * gauss_color_coeff);
        w[i] = v;
        if (v < 0.001)
        {
            emax = i;
            break;
        }
    }
    for (int i = emax; i < 256 * 256 * src.channels(); i++)
        w[i] = 0.0;

    if (src.channels() == 3)//3ͨ��
    {
        const int cstep = (int)im.step - templeteWindowSize * 3;
        const int csstep = (int)im.step - searchWindowSize * 3;
        #pragma omp parallel for
        for (int j = 0; j < src.rows; j++)
        { //j for rows
            uchar* d = dest.ptr(j);
            int* ww = new int[D];       //D Ϊ�������е�Ԫ��������ww���ڼ�¼������ÿ��������򷽲�
            double* nw = new double[D]; //���ݷ����С��˹��Ȩ��һ�����Ȩ��
            for (int i = 0; i < src.cols; i++)
            { //i for cols
                double tweight = 0.0;
                //search loop
                uchar* tprt = im.data + im.step * (sr + j) + 3 * (sr + i);
                uchar* sptr2 = im.data + im.step * j + 3 * i;
                for (int l = searchWindowSize, count = D - 1; l--;)
                {
                    uchar* sptr = sptr2 + im.step * (l);
                    for (int k = searchWindowSize; k--;)
                    {
                        //templete loop
                        int e = 0;
                        uchar* t = tprt;
                        uchar* s = sptr + 3 * k;
                        for (int n = templeteWindowSize; n--;)
                        {
                            for (int m = templeteWindowSize; m--;)
                            {
                                // computing color L2 norm
                                e += (s[0] - t[0]) * (s[0] - t[0]) + (s[1] - t[1]) * (s[1] - t[1]) + (s[2] - t[2]) * (s[2] - t[2]); //L2 norm
                                s += 3;
                                t += 3;
                            }
                            t += cstep;
                            s += cstep;
                        }
                        const int ediv = e * tdiv;
                        ww[count--] = ediv;
                        //����Ȩ��
                        tweight += w[ediv];
                    }
                }
                //Ȩ�ع�һ��
                if (tweight == 0.0)
                {
                    for (int z = 0; z < D; z++)
                        nw[z] = 0;
                    nw[H] = 1;
                }
                else
                {
                    double itweight = 1.0 / (double)tweight;
                    for (int z = 0; z < D; z++)
                        nw[z] = w[ww[z]] * itweight;
                }
                double r = 0.0, g = 0.0, b = 0.0;
                uchar* s = im.ptr(j + tr);
                s += 3 * (tr + i);
                for (int l = searchWindowSize, count = 0; l--;)
                {
                    for (int k = searchWindowSize; k--;)
                    {
                        r += s[0] * nw[count];
                        g += s[1] * nw[count];
                        b += s[2] * nw[count++];
                        s += 3;
                    }
                    s += csstep;
                }
                d[0] = saturate_cast<uchar>(r);//��ֹ��ɫ���
                d[1] = saturate_cast<uchar>(g);
                d[2] = saturate_cast<uchar>(b);
                d += 3;
            } //i
            delete[] ww;
            delete[] nw;
            cout << j << "---block for all ---->" << src.rows << endl;
        } //j
    }
    else if (src.channels() == 1)//��Ӧ�ڵ�ͨ��ͼ
    {
        const int cstep = (int)im.step - templeteWindowSize; //������Ƚ�ʱ�����������һ��ĩβ������һ�п�ͷ
        const int csstep = (int)im.step - searchWindowSize;  //������ѭ���У������������һ��ĩβ������һ�п�ͷ
        #pragma omp parallel for
        //��������Ƕ��ѭ��������ÿ��ͼƬ�����ص�
        for (int j = 0; j < src.rows; j++)
        {
            uchar* d = dest.ptr(j);
            int* ww = new int[D];       //D Ϊ�������е�Ԫ��������ww���ڼ�¼������ÿ��������򷽲�
            double* nw = new double[D]; //���ݷ����С��˹��Ȩ��һ�����Ȩ��
            for (int i = 0; i < src.cols; i++)
            {
                //��������Ƕ��ѭ�����������ص㣨i��j�����������
                double tweight = 0.0;
                uchar* tprt = im.data + im.step * (sr + j) + (sr + i); //sr Ϊ���������ľ�
                uchar* sptr2 = im.data + im.step * j + i;
                for (int l = searchWindowSize, count = D - 1; l--;)
                {
                    uchar* sptr = sptr2 + im.step * (l);
                    for (int k = searchWindowSize; k--;)
                    {
                        //��������Ƕ��ѭ��������ÿ�����ص㣨i��j������������������бȽ�
                        int e = 0; //�ۼƷ���
                        uchar* t = tprt;
                        uchar* s = sptr + k;
                        for (int n = templeteWindowSize; n--;)
                        {
                            for (int m = templeteWindowSize; m--;)
                            {
                                // computing color L2 norm
                                e += (*s - *t) * (*s - *t);
                                s++;
                                t++;
                            }
                            t += cstep;
                            s += cstep;
                        }
                        const int ediv = e * tdiv; //tdiv �������һ�ֲ�Ȩ�ش�С
                        ww[count--] = ediv;
                        //get weighted Euclidean distance
                        tweight += w[ediv];
                    }
                }
                //weight normalizationȨ�ع�һ��
                if (tweight == 0.0)
                {
                    for (int z = 0; z < D; z++)
                        nw[z] = 0;
                    nw[H] = 1;
                }
                else
                {
                    double itweight = 1.0 / (double)tweight;
                    for (int z = 0; z < D; z++)
                        nw[z] = w[ww[z]] * itweight;
                }
                double v = 0.0;
                uchar* s = im.ptr(j + tr);
                s += (tr + i);
                for (int l = searchWindowSize, count = 0; l--;)
                {
                    for (int k = searchWindowSize; k--;)
                    {
                        v += *(s++) * nw[count++];
                    }
                    s += csstep;
                }
                *(d++) = saturate_cast<uchar>(v);
            } //i
            delete[] ww;
            delete[] nw;
        } //j
    }
}

void calcPSF(Mat& outputImg, Size filterSize, int len, double theta)
{
    Mat h(filterSize, CV_32F, Scalar(0));
    Point point(filterSize.width / 2, filterSize.height / 2);
    ellipse(h, point, Size(0, cvRound(float(len) / 2.0)), 90.0 - theta,
        0, 360, Scalar(255), FILLED);
    Scalar summa = sum(h);
    outputImg = h / summa[0];
    Mat tmp;
    normalize(outputImg, tmp, 1, 0, NORM_MINMAX);
    imshow("psf", tmp);
}
void fftshift(const Mat& inputImg, Mat& outputImg)
{
    outputImg = inputImg.clone();
    int cx = outputImg.cols / 2;
    int cy = outputImg.rows / 2;
    Mat q0(outputImg, Rect(0, 0, cx, cy));
    Mat q1(outputImg, Rect(cx, 0, cx, cy));
    Mat q2(outputImg, Rect(0, cy, cx, cy));
    Mat q3(outputImg, Rect(cx, cy, cx, cy));
    Mat tmp;
    q0.copyTo(tmp);
    q3.copyTo(q0);
    tmp.copyTo(q3);
    q1.copyTo(tmp);
    q2.copyTo(q1);
    tmp.copyTo(q2);
}
void filter2DFreq(const Mat& inputImg, Mat& outputImg, const Mat& H)
{
    Mat planes[2] = { Mat_<float>(inputImg.clone()), Mat::zeros(inputImg.size(), CV_32F) };
    Mat complexI;
    merge(planes, 2, complexI);
    dft(complexI, complexI, DFT_SCALE);
    Mat planesH[2] = { Mat_<float>(H.clone()), Mat::zeros(H.size(), CV_32F) };
    Mat complexH;
    merge(planesH, 2, complexH);
    Mat complexIH;
    mulSpectrums(complexI, complexH, complexIH, 0);
    idft(complexIH, complexIH);
    split(complexIH, planes);
    outputImg = planes[0];
}
void calcWnrFilter(const Mat& input_h_PSF, Mat& output_G, double nsr)
{
    Mat h_PSF_shifted;
    fftshift(input_h_PSF, h_PSF_shifted);
    Mat planes[2] = { Mat_<float>(h_PSF_shifted.clone()), Mat::zeros(h_PSF_shifted.size(), CV_32F) };
    Mat complexI;
    merge(planes, 2, complexI);
    dft(complexI, complexI);
    split(complexI, planes);
    Mat denom;
    pow(abs(planes[0]), 2, denom);
    denom += nsr;
    divide(planes[0], denom, output_G);
}
void edgetaper(const Mat& inputImg, Mat& outputImg, double gamma, double beta)
{
    int Nx = inputImg.cols;
    int Ny = inputImg.rows;
    Mat w1(1, Nx, CV_32F, Scalar(0));
    Mat w2(Ny, 1, CV_32F, Scalar(0));
    float* p1 = w1.ptr<float>(0);
    float* p2 = w2.ptr<float>(0);
    float dx = float(2.0 * CV_PI / Nx);
    float x = float(-CV_PI);
    for (int i = 0; i < Nx; i++)
    {
        p1[i] = float(0.5 * (tanh((x + gamma / 2) / beta) - tanh((x - gamma / 2) / beta)));
        x += dx;
    }
    float dy = float(2.0 * CV_PI / Ny);
    float y = float(-CV_PI);
    for (int i = 0; i < Ny; i++)
    {
        p2[i] = float(0.5 * (tanh((y + gamma / 2) / beta) - tanh((y - gamma / 2) / beta)));
        y += dy;
    }
    Mat w = w2 * w1;
    multiply(inputImg, w, outputImg);
}

int main() {
    cv::Mat originalImage = cv::imread("Resources\\input512.jpg", IMREAD_GRAYSCALE);
    cv::Mat degradedImage = originalImage.clone();
    cv::Mat restoredImage;

    // ���ʴ���
    addGaussianNoise(degradedImage, 0, 25);
    //gaussianBlur(degradedImage, 5);

	// ȥģ������
    //Rect roi(0, 0, degradedImage.cols & -2, degradedImage.rows & -2); // �ü�ͼ��ߴ�Ϊż��
    //degradedImage = degradedImage(roi);
    //int LEN = 50;      // ģ������
    //double THETA = 30; // ģ���Ƕ�
    //double snr = 8000; // �����
    //Mat h_PSF, Hw, imgOut;
    //calcPSF(h_PSF, roi.size(), LEN, THETA);
    //calcWnrFilter(h_PSF, Hw, 1.0 / snr);
    //degradedImage.convertTo(degradedImage, CV_32F);
    //filter2DFreq(degradedImage, imgOut, Hw);
    //imgOut.convertTo(imgOut, CV_8U);
    //normalize(imgOut, imgOut, 0, 255, NORM_MINMAX);
    //imshow("Deblurred Image", imgOut);
    //imwrite("deblurred_result.jpg", imgOut);
    //waitKey(0);


    // ��ԭ����
	//cv::fastNlMeansDenoisingColored(degradedImage, restoredImage, 10, 10, 7, 21);
	nonlocalMeansFilter(degradedImage, restoredImage, 7, 21, 10, 0.0);  // �Ǿֲ���ֵ�˲�ȥ��


    // ��ʾ���
    cv::imshow("Original Image", originalImage);
    cv::imshow("Degraded Image", degradedImage);
    cv::imshow("Restored Image", restoredImage);
    cv::waitKey(0);

    // ������
    cv::imwrite("Outputs\\degraded_image.jpg", degradedImage);
    cv::imwrite("Outputs\\restored_image.jpg", restoredImage);

    return 0;
}
