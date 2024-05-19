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

#define Optimal 0.5

// pylon 객체 사용을 위한 네임스페이스.
// cout 사용을 위한 네임스페이스.
// cv 객체 사용을 위한 네임스페이스
// std 사용을 위한 네임스페이스
using namespace Pylon;
using namespace cv;
using namespace std;

// 캡처할 이미지의 수를 정의.
static const uint32_t c_countOfImagesToGrab = 10000;


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
    cout << endl << "시리얼 포트를 닫습니다 " << endl;
    CloseHandle(hSerial);
}

void Bright(int bright) {
    std::cerr << "Bright : " << bright << endl << endl;
}

int Light_Controll_Bright(HANDLE hSerial, int bright, int Similarity) {
    if (Similarity <= Optimal) {
        ++bright;
    }
    BYTE commandC[] = { 0x43, 0x31, static_cast<BYTE>(bright) };  // 채널 1, 데이터 250, 출력 ON
    if (!WriteToSerialPort(hSerial, commandC, sizeof(commandC))) {
        std::cerr << "Error write to serial port: " << GetLastError() << std::endl;
        exit(1);
    }
    Bright(bright);


    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 밝기 변화 시간 간격 ms

    //std::cerr << "Success Write to serial port!  " << "Bright: " << bright << endl;

   //
   //BYTE commandC[] = { 0x43, 0x31, static_cast<BYTE>(bright) };  // 채널 1, 데이터 250, 출력 ON
   //if (!WriteToSerialPort(hSerial, commandC, sizeof(commandC))) {
   //    std::cerr << "Error write to serial port: " << GetLastError() << std::endl;
   //    exit(1);
   //}
   ////std::cerr << "Success Write to serial port!  " << "Bright: " << bright << endl;
   //Bright(bright);
   //bright++;
   //std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 밝기 변화 시간 간격 ms

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

    // pylon 메소드를 사용하기 전에 pylon 런타임을 초기화해야 함.
    PylonInitialize();

    //템블릿 이미지 저장
    Mat templ = imread("templ_04_10.png", cv::IMREAD_GRAYSCALE); //templ용
    if (templ.empty())
    {
        std::cerr << "templ 이미지 없음" << std::endl;
        return 1;
    }
    Mat src_compare = imread("compare_src.png", cv::IMREAD_GRAYSCALE); //compare용
    if (src_compare.empty())
    {
        std::cerr << "compare 이미지 없음" << std::endl;
        return 1;
    }

    try
    {
        // 첫 번째로 찾은 카메라 장치로 인스턴트 카메라 객체 생성.
        CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

        // MaxNumBuffer 파라미터는 캡처를 위해 할당된 버퍼의 수를 제어하는 데 사용될 수 있음.
        // 이 파라미터의 기본값은 10.
        camera.MaxNumBuffer = 5;
        camera.GetInstantCameraNodeMap();
        // c_countOfImagesToGrab 이미지의 캡처 시작.
        // 카메라 장치는 연속 취득을 설정하는 기본 구성으로 파라미터화됨.
        camera.StartGrabbing();

        // 이 스마트 포인터는 캡처 결과 데이터를 받게 됨.
        CGrabResultPtr ptrGrabResult;
        Mat test, src;
        Mat test2, src2;
        double Similarity = 0.0;
        // 최소 gain값 설정
        //camera.GainRaw.SetValue(100); //100이 가장 낮은 Gain값

        CImageFormatConverter formatConverter;
        formatConverter.OutputPixelFormat = PixelType_BGR8packed;
        CPylonImage pylonImage;

        OpenSerialPort(hSerial, L"COM4");  // 포트시리얼, 포트이름 설정

        // 시작하면 초기 밝기 0으로 맞춰주고 시작
        //int형이지만 저장안해서 밝기 0으로 시작가능
        Light_Controll_Bright(hSerial, bright, Similarity);

        // c_countOfImagesToGrab 이미지가 검색되었을 때 Camera.StopGrabbing()이 자동으로 호출됨.
        while (camera.IsGrabbing())
        {
            // 이미지를 기다린 다음 검색. 5000ms의 타임아웃 사용.
            camera.RetrieveResult(500, ptrGrabResult, TimeoutHandling_ThrowException);

            // 이미지가 성공적으로 캡처되었는가?
            if (ptrGrabResult->GrabSucceeded())
            {
                // 이미지 데이터에 접근.
                const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                //cout << "SizeX : " << ptrGrabResult->GetWidth() << endl;
                //cout << "SizeY : " << ptrGrabResult->GetHeight() << endl;
                cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl;

                // 이미지 데이터 변환 후 그레이 스케일 변환 작업
                // pylonImage에서 ptrGrabResult로 이미지 데이터를 변환하는 작업
                formatConverter.Convert(pylonImage, ptrGrabResult);

                // ptrGrabResult에서 가져온 이미지 데이터를 Mat형식으로 변환
                Mat src = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)pylonImage.GetBuffer());

                //src이미지를 GRAY 스케일로 변환
                cv::cvtColor(src, src, COLOR_BGR2GRAY); //GRAY로 받아줌

                Mat img_out;
                src.copyTo(img_out); //src을 img_out으로 얕은 복사, 패턴매칭 하기위함

                start = clock(); //시간체크 시작

                // 원본 이미지에서 탬플릿 이미지와 일치하는 영역을 찾는 알고리즘
                matchTemplate(src, templ, result, method);
                // normalize를 이용해서 이미지 정규화, 필터의 종류, 0~1까지 분포
                normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

                // 주어진 행력의 최소값, 최대값을 찾는 함수로 최소값, 최대값이 있는 좌표정보도 함께 알아낼 수 있음
                // Val -> 값 표시,     Loc -> 좌표표시
                minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

                //작동시간 체크 이후 GRAY로 변경, 그 뒤 작동시간 체크, geometric, masking기법 사용
                for (int i = 0; i < result.rows; i++) {
                    for (int j = 0; j < result.cols; j++) {
                        if (result.at<float>(i, j) >= threshold_min && result.at<float>(i, j) <= threshold_max) {
                            // 임계값 이상의 좌표에 template(pattern) 크기만큼을 더해 사각형을 그림.
                            // OpenCV의 경우 RGB 순서가 아닌 BGR 순서로 표시함.
                            //rectangle(img_out, Point(j, i), Point(j + templ.cols, i + templ.rows), Scalar(0, 0, 255), 1);
                            histogram[(int)img_out.at<uchar>(i, j)]++;

                        }
                    }
                }
                end = clock(); //시간체크 끝

                double searching_time = difftime(end, start) / CLOCKS_PER_SEC;

                Mat src_hist, src_hist_result, templ_hist, templ_hist_img;
                Mat src_compare_hist, src_compare_result;
                Mat threshold_img, threshold_hist, threshold_hist_result; // 히스토그램의 이진화


                //calc_Histo(src, src_hist, histsize, range); // 히스토그램 계산
                //draw_histo(src_hist, src_hist_result); //히스토그램 그래프 그리기

                //calc_Histo(src_compare, src_compare_hist, histsize, range); // 히스토그램 계산
                //draw_histo(src_compare_hist, src_compare_result); //히스토그램 그래프 그리기

                //위 코드 함수, 히스토그램 계산 + 그리기
                create_hist(src, src_hist, src_hist_result);
                create_hist(src_compare, src_compare_hist, src_compare_result);

                /*int x = 0, y = 0;
                int pixelvalue = src.at<uchar>(y, x);

                std::cout << "pixel value at (x,y) : " << pixelvalue << std::endl;
                */

                // 추후 조건문로 유사도 일정 값 넘어가면 종료
                // compareHist니까 src의 히스토그램, src_compare의 히스토그램을 비교 후 유사율 출력
                Similarity = cv::compareHist(src_hist, src_compare_hist, HISTCMP_CORREL);
                cout << "Similarity : " << Similarity << endl;


                if (Similarity <= Optimal) { // 유사도가 일정이상일 경우 밝기만 출력
                    bright = Light_Controll_Bright(hSerial, bright, Similarity);
                }

                /*
                else if (Similarity > Optimal)
                {
                    CloseSerialPort(hSerial);
                }
                */

                //else // 유사도가 일정 이하일경우에만 실행 





                //if (Similarity >= 0.5) // 유사도 일정이상 높을 경우 다음 작업 실행
                //{
                //    continue;
                //}
                //else if (Similarity < 0.5) { // 유사도가 일정 이하일 경우 light controller을 사용하여 밝기 +1
                //    // 커맨드 전송 예제
                //    
                //    BYTE commandC[] = { 0x43, 0x31, static_cast<BYTE>(bright) };  // 채널 1, 데이터 250, 출력 ON
                //    if (!WriteToSerialPort(hSerial, commandC, sizeof(commandC))) {
                //        std::cerr << "Error write to serial port: " << GetLastError() << std::endl;
                //        exit(1);
                //    } std::cerr << "Success Write to serial port!  " << "Bright: " << bright << endl;
                //    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 밝기 변화 시간 간격 ms
                //    bright++;
                //    //  if (i >= value) { break; } // 임계값에 다다르면 밝기 변화 정지
                //}


                //create_hist(threshold_img, threshold_hist, threshold_hist_result); //마스킹된 이미지 그리기

                // 히스토그램 2개 compareHist()로 비교해보기
                // int method 
                // 1, cv2.HISTCMP_CORREL : 상관관계
                // 1 : 완전일치, -1 : 완전 불일치, 0 : 무관계
                // 빠르지만 부정확
                // 
                // 2. cv2.HISTCMP_CHISQR : 카이제곱 검정
                // 0 : 완전 일치, 무한대 : 완전 불일치
                // 
                // 3. cvHISTCMP_INTERSECT : 교차
                // 1 : 완전 일치, 0: 완전 불일치(히스토그램이 1로 정규화된 경우)
                // 
                // 4. cv2.HISTCMP_BHATTACHARYYA " 밭타차야 거리
                // 0 : 완전 일치, 1: 완전 불일치
                // 느리지만 가장 정확
                // 
                // 5. EMD
                // 직관적이지만 가장 느림
                // 
                //compareHist(src_hist_img, templ_hist_img, 1); //hist1, hist2, int method
                //이 뒤 해당하는 값에 따라 light controller, gain, exposer 값 수정

                //출력
                imshow("src", img_out);
                imshow("compare_src", src_compare);
                imshow("templ", templ);
                imshow("result", result);
                imshow("hist", src_hist_result);
                imshow("Compare Histogram", src_compare_result);


                waitKey(1);

#ifdef PYLON_WIN_BUILD
                // 캡처된 이미지를 표시.
                //Pylon::DisplayImage(1, ptrGrabResult);
#endif
            }
            else
            {
                // 오류 메시지 출력.
                cout << "Error 쵸비상: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
        }
    }
    catch (const GenericException& e)
    {
        // 예외 처리.
        cerr << "An exception occurred." << endl
            << e.GetDescription() << endl;
        int exitCode = 1;
    }

    // 종료 전에 대기를 비활성화하려면 다음 두 줄을 주석 처리.
    cerr << endl << "Press enter to exit." << endl;
    while (cin.get() != '\n');

    // 모든 pylon 리소스를 해제.
    //PylonTerminate();

    return 1;
}