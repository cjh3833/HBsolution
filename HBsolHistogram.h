/*
* 2018848045 최재형 HB솔루션
*/

// pylon API를 사용하기 위한 헤더 파일 포함.
#include <pylon/PylonIncludes.h>
// openCV를 사용하기 위한 헤더파일
#include <opencv2/opencv.hpp>
//time을 쓰기 위한 헤더파일
#include <time.h>
#include <windows.h>
#include <iostream>
#include <iomanip> // 10진수 -> 16진수
#include <chrono>
#include <thread>

#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h> // Windows 환경에서 GUI 관련 기능을 사용하기 위한 헤더 파일.
#endif

#define Optimal 0.7

using namespace Pylon;
using namespace cv;
using namespace std;

//히스토그램 계산 함수
void calc_Histo(const Mat& img, Mat& hist, int bins, int range_max = 256)
{
    int histSize[] = { 256 }; // 히스토그램 계급 개수
    float range[] = { 0, (float)range_max }; // 0번 채널 화소값 범위
    int channels[] = { 0 }; //채널 목록 - 단일 채널
    const float* ranges[] = { range }; //모든 채널 화소 범위

    calcHist(&img, 1, channels, Mat(), hist, 1, histSize, ranges);
}


//히스토그램 드로잉 함수
void draw_histo(Mat hist, Mat& hist_img, Size size = Size(256, 200)) {
    hist_img = Mat(size, CV_8U, Scalar(255)); //그래프 행렬
    float bin = (float)hist_img.cols / hist.rows; // 한 계급 너비
    normalize(hist, hist, 0, hist_img.rows, NORM_MINMAX); // 정규화

    for (int i = 0; i < hist.rows; i++)
    {
        float start_x = i * bin; // 막대 사각형 시작 X 좌표
        float end_x = (i + 1) * bin; // 막대 사각형 종료 x 좌표
        Point2f pt1(start_x, 0);
        Point2f pt2(end_x, hist.at<float>(i));

        if (pt2.y > 0)
            rectangle(hist_img, pt1, pt2, Scalar(0), -1); // 막대 사각형 그리기
    }
    flip(hist_img, hist_img, 0); // x축 기준 영상 뒤집기
}


//히스토그램 그리는 클래스
void create_hist(Mat img, Mat& hist, Mat& hist_img)
{
    int histsize = 256, range = 256;
    calc_Histo(img, hist, histsize, range); // 히스토그램 계산
    draw_histo(hist, hist_img); //히스토그램 그래프 그리기
}

// 1. 시리얼 포트 열기
void OpenSerialPort(HANDLE& hSerial, LPCWSTR portName) { // 매개변수로 핸들, 포트번호
    hSerial = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    // createFile로 조명파일 생성

    if (hSerial == INVALID_HANDLE_VALUE) { // 오류발생시 출력 후 종료
        std::cerr << "Error opening serial port: " << GetLastError() << std::endl;
        exit(1);
    }
    std::cerr << "Success opening serial port!" << endl; // 포트 연결 성공시 출력

    /*
    ERROR_FILE_NOT_FOUND (2) - 지정된 파일이나 디바이스를 찾을 수 없음 (잘못된 포트 이름 또는 없는 포트).
    ERROR_ACCESS_DENIED (5) - 접근 권한이 거부됨 (포트가 다른 프로세스에 의해 사용 중이거나 권한 부족).
    ERROR_INVALID_PARAMETER (87) - 함수에 잘못된 매개변수가 전달됨 (인자 오류).
    ERROR_GEN_FAILURE (31) - 장치에 연결된 일반적인 오류 (하드웨어 또는 드라이버 문제).
    ERROR_SHARING_VIOLATION (32) - 파일 또는 리소스 접근 공유 위반 (다른 프로세스/스레드가 사용 중).
    ERROR_OPERATION_ABORTED (995) - 외부에서 중단된 입출력 작업 (작업 취소됨).
    */

    DCB dcbSerialParams = { 0 };  //Serial통신에 필요한 구조체
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams); // 통신에 필요한 sizeof 받아두나 ?
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error getting state: " << GetLastError() << std::endl;
        CloseHandle(hSerial);
        exit(1);
    } std::cerr << "Success getting state!" << endl;

    //dcbSerialParams 구조체에 속상 값 부여(우리가 쓰는 조명 장치에 원래 사용되어야 하는 기본 값들)
    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8; // 전송 데이터는 3바이트의 문자
    dcbSerialParams.StopBits = ONESTOPBIT; // 스탑 비트 1
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error setting serial port state: " << GetLastError() << std::endl;
        CloseHandle(hSerial);
        exit(1);
    }
    std::cerr << "Success setting serial port state!" << endl;
}


// 2. 데이터 전송 - 커맨드를 바이트로 변환하여 시리얼 포트를 통해 전송합니다.WriteFile 함수를 사용하여 데이터를 전송합니다.
bool WriteToSerialPort(HANDLE hSerial, const BYTE* buffer, DWORD bytesToWrite) {
    DWORD bytesWritten;
    //WriteFile() 함수를 통해 전송
    BOOL result = WriteFile(hSerial, buffer, bytesToWrite, &bytesWritten, NULL);
    return result && bytesWritten == bytesToWrite;
}


// 3. 데이터 수신 - ReadFile 함수를 사용하여 시리얼 포트로부터 데이터를 읽습니다.
bool ReadFromSerialPort(HANDLE hSerial, BYTE* buffer, DWORD bufferSize, DWORD& bytesRead) {
    BOOL result = ReadFile(hSerial, buffer, bufferSize, &bytesRead, NULL);
    return result != 0;
}


// 4. 시리얼 포트 닫기 - 통신이 완료되면 시리얼 포트를 닫습니다. 
void CloseSerialPort(HANDLE hSerial) {
    cout << "닫는다 2" << endl;
    CloseHandle(hSerial);
}

void Bright(int bright) {
    std::cerr << "Bright : " << bright << endl << endl;
}

int Light_Controll_Bright(HANDLE hSerial, int bright, double Similarity) {
    bright++;
    BYTE commandC[] = { 0x43, 0x31, static_cast<BYTE>(bright) };  // 채널 1, 데이터 250, 출력 ON
    if (!WriteToSerialPort(hSerial, commandC, sizeof(commandC))) {
        std::cerr << "Error write to serial port: " << GetLastError() << std::endl;
        exit(1);
    }

    Bright(bright);
    //std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 밝기 변화 시간 간격 ms

    return bright;

    //std::cerr << "Success Write to serial port!  " << "Bright: " << bright << endl;

    //BYTE commandC[] = { 0x43, 0x31, static_cast<BYTE>(bright) };  // 채널 1, 데이터 250, 출력 ON
    //if (!WriteToSerialPort(hSerial, commandC, sizeof(commandC))) {
    //    std::cerr << "Error write to serial port: " << GetLastError() << std::endl;
    //    exit(1);
    //}
    ////std::cerr << "Success Write to serial port!  " << "Bright: " << bright << endl;
    //Bright(bright);
    //bright++;
    //std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 밝기 변화 시간 간격 ms
}