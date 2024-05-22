/*
* 2018848045 ������ HB�ַ��
*/

// pylon API�� ����ϱ� ���� ��� ���� ����.
#include <pylon/PylonIncludes.h>
// openCV�� ����ϱ� ���� �������
#include <opencv2/opencv.hpp>
//time�� ���� ���� �������
#include <time.h>
#include <windows.h>
#include <iostream>
#include <iomanip> // 10���� -> 16����
#include <chrono>
#include <thread>
#include "HBsolHistogram.h"


#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h> // Windows ȯ�濡�� GUI ���� ����� ����ϱ� ���� ��� ����.
#endif

#define Optimal 0.7

// pylon ��ü ����� ���� ���ӽ����̽�.
// cv ��ü ����� ���� ���ӽ����̽�
// std ����� ���� ���ӽ����̽�
using namespace Pylon;
using namespace cv;
using namespace std;

// ĸó�� �̹����� ���� ����.
static const uint32_t c_countOfImagesToGrab = 300;


int main(int /*argc*/, char* /*argv*/[])
{
    HANDLE hSerial;
    Point minLoc, maxLoc, matchLoc;
    Mat result;
    clock_t start, end;

    int method = 1, histsize = 256, range = 256, histogram[256] = { 0 };
    int  bright = 0;
    double minVal, maxVal;
    double threshold_max = 0.05;
    double threshold_min = 0.02;

    // pylon �޼ҵ带 ����ϱ� ���� pylon ��Ÿ���� �ʱ�ȭ�ؾ� ��.
    PylonInitialize();

    //�ۺ� �̹��� ����
    Mat templ = imread("templ_04_10.png", cv::IMREAD_GRAYSCALE); //templ��
    if (templ.empty()) {
        std::cerr << "templ �̹��� ����" << std::endl;
        return 1;
    }

    Mat src_compare = imread("compare_src.png", cv::IMREAD_GRAYSCALE); //compare��
    if (src_compare.empty()) {
        std::cerr << "compare �̹��� ����" << std::endl;
        return 1;
    }
    try
    {
        // ù ��°�� ã�� ī�޶� ��ġ�� �ν���Ʈ ī�޶� ��ü ����.
        CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

        // MaxNumBuffer �Ķ���ʹ� ĸó�� ���� �Ҵ�� ������ ���� �����ϴ� �� ���� �� ����.
        // �� �Ķ������ �⺻���� 10.
        camera.MaxNumBuffer = 5;
        camera.GetInstantCameraNodeMap();
        // c_countOfImagesToGrab �̹����� ĸó ����.
        // ī�޶� ��ġ�� ���� ����� �����ϴ� �⺻ �������� �Ķ����ȭ��.
        camera.StartGrabbing();

        // �� ����Ʈ �����ʹ� ĸó ��� �����͸� �ް� ��.
        CGrabResultPtr ptrGrabResult;
        Mat test, src;
        Mat src2;
        double Similarity = 0;

        CImageFormatConverter formatConverter;
        formatConverter.OutputPixelFormat = PixelType_BGR8packed;
        CPylonImage pylonImage;

        OpenSerialPort(hSerial, L"COM4");  // ��Ʈ�ø���, ��Ʈ�̸� ����

        // �����ϸ� �ʱ� ��� 0���� �����ְ� ����
        //int�������� ������ؼ� ��� 0���� ���۰���
        Light_Controll_Bright(hSerial, bright, Similarity);

        // c_countOfImagesToGrab �̹����� �˻��Ǿ��� �� Camera.StopGrabbing()�� �ڵ����� ȣ���.
        while (camera.IsGrabbing())
        {
            start = clock(); //�ð�üũ ����

            // �̹����� ��ٸ� ���� �˻�. 500ms�� Ÿ�Ӿƿ� ���.
            camera.RetrieveResult(500, ptrGrabResult, TimeoutHandling_ThrowException);

            // �̹����� ���������� ĸó�Ǿ��°�?
            if (ptrGrabResult->GrabSucceeded())
            {
                // �̹��� �����Ϳ� ����.
                const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl;

                // �̹��� ������ ��ȯ �� �׷��� ������ ��ȯ �۾�
                // pylonImage���� ptrGrabResult�� �̹��� �����͸� ��ȯ�ϴ� �۾�
                formatConverter.Convert(pylonImage, ptrGrabResult);

                // ptrGrabResult���� ������ �̹��� �����͸� Mat�������� ��ȯ
                Mat src = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)pylonImage.GetBuffer());

                //src�̹����� GRAY �����Ϸ� ��ȯ
                cv::cvtColor(src, src, COLOR_BGR2GRAY); //GRAY�� �޾���

                Mat img_out;
                src.copyTo(img_out); //src�� img_out���� ���� ����, ���ϸ�Ī �ϱ�����

                // ���� �̹������� ���ø� �̹����� ��ġ�ϴ� ������ ã�� �˰���
                matchTemplate(src, templ, result, method);
                // normalize�� �̿��ؼ� �̹��� ����ȭ, ������ ����, 0~1���� ����
                normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

                // �־��� ����� �ּҰ�, �ִ밪�� ã�� �Լ��� �ּҰ�, �ִ밪�� �ִ� ��ǥ������ �Բ� �˾Ƴ� �� ����
                // Val -> �� ǥ��,     Loc -> ��ǥǥ��
                minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

                //�۵��ð� üũ ���� GRAY�� ����, �� �� �۵��ð� üũ, geometric, masking��� ���
                for (int i = 0; i < result.rows; i++) {
                    for (int j = 0; j < result.cols; j++) {
                        if (result.at<float>(i, j) >= threshold_min && result.at<float>(i, j) <= threshold_max) {
                            // �Ӱ谪 �̻��� ��ǥ�� template(pattern) ũ�⸸ŭ�� ���� �簢���� �׸�.
                            // OpenCV�� ��� RGB ������ �ƴ� BGR ������ ǥ����.
                            //rectangle(img_out, Point(j, i), Point(j + templ.cols, i + templ.rows), Scalar(0, 0, 255), 1);
                            histogram[(int)img_out.at<uchar>(i, j)]++;
                        }
                    }
                }

                Mat src_hist, src_hist_result, templ_hist, templ_hist_img;
                Mat src_compare_hist, src_compare_result;
                Mat threshold_img, threshold_hist, threshold_hist_result; // ������׷��� ����ȭ

                //calc_Histo(src, src_hist, histsize, range); // ������׷� ���
                //draw_histo(src_hist, src_hist_result); //������׷� �׷��� �׸���

                //calc_Histo(src_compare, src_compare_hist, histsize, range); // ������׷� ���
                //draw_histo(src_compare_hist, src_compare_result); //������׷� �׷��� �׸���

                //�� �ڵ� �Լ�, ������׷� ��� + �׸���
                create_hist(src, src_hist, src_hist_result);
                create_hist(src_compare, src_compare_hist, src_compare_result);

                /*int x = 0, y = 0;
                int pixelvalue = src.at<uchar>(y, x);

                std::cout << "pixel value at (x,y) : " << pixelvalue << std::endl;
                */

                //Similarity = cv::compareHist(src_hist, src_compare_hist, HISTCMP_CORREL);
                //cout << "Similarity : " << Similarity << endl;


                if (Similarity < Optimal) {
                    Similarity = cv::compareHist(src_hist, src_compare_hist, HISTCMP_CORREL);

                    bright = Light_Controll_Bright(hSerial, bright, Similarity);
                    cout << "Similarity : " << Similarity << endl;
                }

                else {
                    Bright(bright);
                    cout << "Similarity : " << Similarity << endl;
                }

                //    // Ŀ�ǵ� ���� ����
                //    
                //    BYTE commandC[] = { 0x43, 0x31, static_cast<BYTE>(bright) };  // ä�� 1, ������ 250, ��� ON
                //    if (!WriteToSerialPort(hSerial, commandC, sizeof(commandC))) {
                //        std::cerr << "Error write to serial port: " << GetLastError() << std::endl;
                //        exit(1);
                //    } std::cerr << "Success Write to serial port!  " << "Bright: " << bright << endl;
                //    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // ��� ��ȭ �ð� ���� ms
                //    bright++;
                //    //  if (i >= value) { break; } // �Ӱ谪�� �ٴٸ��� ��� ��ȭ ����
                //}

                //create_hist(threshold_img, threshold_hist, threshold_hist_result); //����ŷ�� �̹��� �׸���

                // ������׷� 2�� compareHist()�� ���غ���
                // int method 
                // 1, cv2.HISTCMP_CORREL : �������
                // 1 : ������ġ, -1 : ���� ����ġ, 0 : ������
                // �������� ����Ȯ
                // 
                // 2. cv2.HISTCMP_CHISQR : ī������ ����
                // 0 : ���� ��ġ, ���Ѵ� : ���� ����ġ
                // 
                // 3. cvHISTCMP_INTERSECT : ����
                // 1 : ���� ��ġ, 0: ���� ����ġ(������׷��� 1�� ����ȭ�� ���)
                // 
                // 4. cv2.HISTCMP_BHATTACHARYYA " ��Ÿ���� �Ÿ�
                // 0 : ���� ��ġ, 1: ���� ����ġ
                // �������� ���� ��Ȯ
                // 
                // 5. EMD
                // ������������ ���� ����
                // 

                //���
                imshow("templ", templ);
                imshow("result", result);
                imshow("hist", src_hist_result);
                imshow("Compare Histogram", src_compare_result);
                imshow("compare_src", src_compare);
                imshow("src", img_out);

                end = clock();

                cout << "�ҿ�ð� : " << difftime(end, start) / CLOCKS_PER_SEC << endl;

                waitKey(1);

#ifdef PYLON_WIN_BUILD
                // ĸó�� �̹����� ǥ��.
                //Pylon::DisplayImage(1, ptrGrabResult);
#endif
            }
            else
            {
                // ���� �޽��� ���.
                cout << "Error �ݺ��: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
        }
    }
    catch (const GenericException& e)
    {
        // ���� ó��.
        cerr << "An exception occurred." << endl
            << e.GetDescription() << endl;
        int exitCode = 1;
    }

    // ���� ���� ��⸦ ��Ȱ��ȭ�Ϸ��� ���� �� ���� �ּ� ó��.
    cerr << endl << "Press enter to exit." << endl;
    while (cin.get() != '\n');

    // ��� pylon ���ҽ��� ����.
    //PylonTerminate();

    return 1;
}