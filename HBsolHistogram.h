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

#define Optimal 0.7

using namespace Pylon;
using namespace cv;
using namespace std;

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
    cout << "�ݴ´� 2" << endl;
    CloseHandle(hSerial);
}

void Bright(int bright) {
    std::cerr << "Bright : " << bright << endl << endl;
}

int Light_Controll_Bright(HANDLE hSerial, int bright, double Similarity) {
    bright++;
    BYTE commandC[] = { 0x43, 0x31, static_cast<BYTE>(bright) };  // ä�� 1, ������ 250, ��� ON
    if (!WriteToSerialPort(hSerial, commandC, sizeof(commandC))) {
        std::cerr << "Error write to serial port: " << GetLastError() << std::endl;
        exit(1);
    }

    Bright(bright);
    //std::this_thread::sleep_for(std::chrono::milliseconds(100)); // ��� ��ȭ �ð� ���� ms

    return bright;

    //std::cerr << "Success Write to serial port!  " << "Bright: " << bright << endl;

    //BYTE commandC[] = { 0x43, 0x31, static_cast<BYTE>(bright) };  // ä�� 1, ������ 250, ��� ON
    //if (!WriteToSerialPort(hSerial, commandC, sizeof(commandC))) {
    //    std::cerr << "Error write to serial port: " << GetLastError() << std::endl;
    //    exit(1);
    //}
    ////std::cerr << "Success Write to serial port!  " << "Bright: " << bright << endl;
    //Bright(bright);
    //bright++;
    //std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // ��� ��ȭ �ð� ���� ms
}