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

#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h> // Windows ȯ�濡�� GUI ���� ����� ����ϱ� ���� ��� ����.
#endif

#define Optimal 0.5

// pylon ��ü ����� ���� ���ӽ����̽�.
// cout ����� ���� ���ӽ����̽�.
// cv ��ü ����� ���� ���ӽ����̽�
// std ����� ���� ���ӽ����̽�
using namespace Pylon;
using namespace cv;
using namespace std;

// ĸó�� �̹����� ���� ����.
static const uint32_t c_countOfImagesToGrab = 10000;


//������׷� ��� �Լ�
void calc_Histo(const Mat& img, Mat& hist, int bins, int range_max = 256)
{
    int histSize[] = { 256 }; // ������׷� ��� ����
    float range[] = { 0, (float)range_max }; // 0�� ä�� ȭ�Ұ� ����
    int channels[] = { 0 }; //ä�� ��� - ���� ä��
    const float* ranges[] = { range }; //��� ä�� ȭ�� ����

    calcHist(&img, 1, channels, Mat(), hist, 1, histSize, ranges);
}


//������׷� ����� �Լ�
void draw_histo(Mat hist, Mat& hist_img, Size size = Size(256, 200)) {
    hist_img = Mat(size, CV_8U, Scalar(255)); //�׷��� ���
    float bin = (float)hist_img.cols / hist.rows; // �� ��� �ʺ�
    normalize(hist, hist, 0, hist_img.rows, NORM_MINMAX); // ����ȭ

    for (int i = 0; i < hist.rows; i++)
    {
        float start_x = i * bin; // ���� �簢�� ���� X ��ǥ
        float end_x = (i + 1) * bin; // ���� �簢�� ���� x ��ǥ
        Point2f pt1(start_x, 0);
        Point2f pt2(end_x, hist.at<float>(i));

        if (pt2.y > 0)
            rectangle(hist_img, pt1, pt2, Scalar(0), -1); // ���� �簢�� �׸���
    }
    flip(hist_img, hist_img, 0); // x�� ���� ���� ������
}


//������׷� �׸��� Ŭ����
void create_hist(Mat img, Mat& hist, Mat& hist_img)
{
    int histsize = 256, range = 256;
    calc_Histo(img, hist, histsize, range); // ������׷� ���
    draw_histo(hist, hist_img); //������׷� �׷��� �׸���
}

// 1. �ø��� ��Ʈ ����
void OpenSerialPort(HANDLE& hSerial, LPCWSTR portName) { // �Ű������� �ڵ�, ��Ʈ��ȣ
    hSerial = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    // createFile�� �������� ����

    if (hSerial == INVALID_HANDLE_VALUE) { // �����߻��� ��� �� ����
        std::cerr << "Error opening serial port: " << GetLastError() << std::endl;
        exit(1);
    }
    std::cerr << "Success opening serial port!" << endl; // ��Ʈ ���� ������ ���

    /*
    ERROR_FILE_NOT_FOUND (2) - ������ �����̳� ����̽��� ã�� �� ���� (�߸��� ��Ʈ �̸� �Ǵ� ���� ��Ʈ).
    ERROR_ACCESS_DENIED (5) - ���� ������ �źε� (��Ʈ�� �ٸ� ���μ����� ���� ��� ���̰ų� ���� ����).
    ERROR_INVALID_PARAMETER (87) - �Լ��� �߸��� �Ű������� ���޵� (���� ����).
    ERROR_GEN_FAILURE (31) - ��ġ�� ����� �Ϲ����� ���� (�ϵ���� �Ǵ� ����̹� ����).
    ERROR_SHARING_VIOLATION (32) - ���� �Ǵ� ���ҽ� ���� ���� ���� (�ٸ� ���μ���/�����尡 ��� ��).
    ERROR_OPERATION_ABORTED (995) - �ܺο��� �ߴܵ� ����� �۾� (�۾� ��ҵ�).
    */

    DCB dcbSerialParams = { 0 };  //Serial��ſ� �ʿ��� ����ü
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams); // ��ſ� �ʿ��� sizeof �޾Ƶγ� ?
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error getting state: " << GetLastError() << std::endl;
        CloseHandle(hSerial);
        exit(1);
    } std::cerr << "Success getting state!" << endl;

    //dcbSerialParams ����ü�� �ӻ� �� �ο�(�츮�� ���� ���� ��ġ�� ���� ���Ǿ�� �ϴ� �⺻ ����)
    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8; // ���� �����ʹ� 3����Ʈ�� ����
    dcbSerialParams.StopBits = ONESTOPBIT; // ��ž ��Ʈ 1
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error setting serial port state: " << GetLastError() << std::endl;
        CloseHandle(hSerial);
        exit(1);
    }
    std::cerr << "Success setting serial port state!" << endl;
}


// 2. ������ ���� - Ŀ�ǵ带 ����Ʈ�� ��ȯ�Ͽ� �ø��� ��Ʈ�� ���� �����մϴ�.WriteFile �Լ��� ����Ͽ� �����͸� �����մϴ�.
bool WriteToSerialPort(HANDLE hSerial, const BYTE* buffer, DWORD bytesToWrite) {
    DWORD bytesWritten;
    //WriteFile() �Լ��� ���� ����
    BOOL result = WriteFile(hSerial, buffer, bytesToWrite, &bytesWritten, NULL);
    return result && bytesWritten == bytesToWrite;
}


// 3. ������ ���� - ReadFile �Լ��� ����Ͽ� �ø��� ��Ʈ�κ��� �����͸� �н��ϴ�.
bool ReadFromSerialPort(HANDLE hSerial, BYTE* buffer, DWORD bufferSize, DWORD& bytesRead) {
    BOOL result = ReadFile(hSerial, buffer, bufferSize, &bytesRead, NULL);
    return result != 0;
}


// 4. �ø��� ��Ʈ �ݱ� - ����� �Ϸ�Ǹ� �ø��� ��Ʈ�� �ݽ��ϴ�. 
void CloseSerialPort(HANDLE hSerial) {
    cout << endl << "�ø��� ��Ʈ�� �ݽ��ϴ� " << endl;
    CloseHandle(hSerial);
}

void Bright(int bright) {
    std::cerr << "Bright : " << bright << endl << endl;
}

int Light_Controll_Bright(HANDLE hSerial, int bright, int Similarity) {
    if (Similarity <= Optimal) {
        ++bright;
    }
    BYTE commandC[] = { 0x43, 0x31, static_cast<BYTE>(bright) };  // ä�� 1, ������ 250, ��� ON
    if (!WriteToSerialPort(hSerial, commandC, sizeof(commandC))) {
        std::cerr << "Error write to serial port: " << GetLastError() << std::endl;
        exit(1);
    }
    Bright(bright);


    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // ��� ��ȭ �ð� ���� ms

    //std::cerr << "Success Write to serial port!  " << "Bright: " << bright << endl;

   //
   //BYTE commandC[] = { 0x43, 0x31, static_cast<BYTE>(bright) };  // ä�� 1, ������ 250, ��� ON
   //if (!WriteToSerialPort(hSerial, commandC, sizeof(commandC))) {
   //    std::cerr << "Error write to serial port: " << GetLastError() << std::endl;
   //    exit(1);
   //}
   ////std::cerr << "Success Write to serial port!  " << "Bright: " << bright << endl;
   //Bright(bright);
   //bright++;
   //std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // ��� ��ȭ �ð� ���� ms

    return bright;
}


int main(int /*argc*/, char* /*argv*/[])
{
    HANDLE hSerial;
    Point minLoc, maxLoc, matchLoc;
    Mat result;
    clock_t start, end;

    int method = 1, histsize = 256, range = 256, histogram[256] = { 0 };
    int  bright = 0, time_watch = 0, exitcode = 0;

    double minVal, maxVal;
    double threshold_max = 0.05;
    double threshold_min = 0.02;
    double time_working = 0.0;

    // pylon �޼ҵ带 ����ϱ� ���� pylon ��Ÿ���� �ʱ�ȭ�ؾ� ��.
    PylonInitialize();

    //�ۺ� �̹��� ����
    Mat templ = imread("templ_04_10.png", cv::IMREAD_GRAYSCALE); //templ��
    if (templ.empty())
    {
        std::cerr << "templ �̹��� ����" << std::endl;
        return 1;
    }
    Mat src_compare = imread("compare_src.png", cv::IMREAD_GRAYSCALE); //compare��
    if (src_compare.empty())
    {
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
        Mat test2, src2;
        double Similarity = 0.0;
        // �ּ� gain�� ����
        //camera.GainRaw.SetValue(100); //100�� ���� ���� Gain��

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
            // �̹����� ��ٸ� ���� �˻�. 5000ms�� Ÿ�Ӿƿ� ���.
            camera.RetrieveResult(500, ptrGrabResult, TimeoutHandling_ThrowException);

            // �̹����� ���������� ĸó�Ǿ��°�?
            if (ptrGrabResult->GrabSucceeded())
            {
                // �̹��� �����Ϳ� ����.
                const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                //cout << "SizeX : " << ptrGrabResult->GetWidth() << endl;
                //cout << "SizeY : " << ptrGrabResult->GetHeight() << endl;
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

                start = clock(); //�ð�üũ ����

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
                end = clock(); //�ð�üũ ��

                double searching_time = difftime(end, start) / CLOCKS_PER_SEC;

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

                // ���� ���ǹ��� ���絵 ���� �� �Ѿ�� ����
                // compareHist�ϱ� src�� ������׷�, src_compare�� ������׷��� �� �� ������ ���
                Similarity = cv::compareHist(src_hist, src_compare_hist, HISTCMP_CORREL);
                cout << "Similarity : " << Similarity << endl;


                if (Similarity <= Optimal) { // ���絵�� �����̻��� ��� ��⸸ ���
                    bright = Light_Controll_Bright(hSerial, bright, Similarity);
                }

                /*
                else if (Similarity > Optimal)
                {
                    CloseSerialPort(hSerial);
                }
                */

                //else // ���絵�� ���� �����ϰ�쿡�� ���� 





                //if (Similarity >= 0.5) // ���絵 �����̻� ���� ��� ���� �۾� ����
                //{
                //    continue;
                //}
                //else if (Similarity < 0.5) { // ���絵�� ���� ������ ��� light controller�� ����Ͽ� ��� +1
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
                //compareHist(src_hist_img, templ_hist_img, 1); //hist1, hist2, int method
                //�� �� �ش��ϴ� ���� ���� light controller, gain, exposer �� ����

                //���
                imshow("src", img_out);
                imshow("compare_src", src_compare);
                imshow("templ", templ);
                imshow("result", result);
                imshow("hist", src_hist_result);
                imshow("Compare Histogram", src_compare_result);


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