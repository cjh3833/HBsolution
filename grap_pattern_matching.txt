// pylon API를 사용하기 위한 헤더 파일 포함.
#include <pylon/PylonIncludes.h>
// openCV를 사용하기 위한 헤더파일
#include <opencv2/opencv.hpp> 
//time을 쓰기 위한 헤더파일
#include <time.h> 
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h> // Windows 환경에서 GUI 관련 기능을 사용하기 위한 헤더 파일.
#endif

// pylon 객체 사용을 위한 네임스페이스.
// cout 사용을 위한 네임스페이스.
using namespace Pylon;
using namespace std;


// cv 객체 사용을 위한 네임스페이스
// std 사용을 위한 네임스페이스
using namespace cv;
using namespace std;

// 캡처할 이미지의 수를 정의.
static const uint32_t c_countOfImagesToGrab = 300;

int main(int /*argc*/, char* /*argv*/[])
{
    // 이미지 결과 연산을 시켜줄 변수
    clock_t start, end;
    double minVal, maxVal;
    Point minLoc, maxLoc;
    Point matchLoc;
    Mat result;
    //시간 경과를 보여줄 것
    int time_watch = 0;

    // 샘플 애플리케이션의 종료 코드.
    int exitCode = 0;

    // pylon 메소드를 사용하기 전에 pylon 런타임을 초기화해야 함.
    PylonInitialize();

    //템블릿 이미지 저장
    Mat templ = imread("templ_4_1.png");

    try
    {
        // 첫 번째로 찾은 카메라 장치로 인스턴트 카메라 객체 생성.
        CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

        // 카메라의 모델 이름 출력.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

        // MaxNumBuffer 파라미터는 캡처를 위해 할당된 버퍼의 수를 제어하는 데 사용될 수 있음.
        // 이 파라미터의 기본값은 10.
        camera.MaxNumBuffer = 5;

        // c_countOfImagesToGrab 이미지의 캡처 시작.
        // 카메라 장치는 연속 취득을 설정하는 기본 구성으로 파라미터화됨.
        camera.StartGrabbing(c_countOfImagesToGrab);

        // 이 스마트 포인터는 캡처 결과 데이터를 받게 됨.
        CGrabResultPtr ptrGrabResult;  //캡쳐결과를 받으면 이걸 패턴매칭에 연결시키면 ?
        Mat test, src;
        


        // 왜 ?
        CImageFormatConverter formatConverter;

        formatConverter.OutputPixelFormat = PixelType_BGR8packed;
        CPylonImage pylonImage;

        int i = 0;




        // c_countOfImagesToGrab 이미지가 검색되었을 때 Camera.StopGrabbing()이 자동으로 호출됨.
        while (camera.IsGrabbing())
        {
            //시간경과를 보여주기 위해 증가
            time_watch++;

            // 이미지를 기다린 다음 검색. 5000ms의 타임아웃 사용.
            camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

            // 이미지가 성공적으로 캡처되었는가?
            if (ptrGrabResult->GrabSucceeded())
            {
                //이미지 받아오고 src형변환 후 imshow
                //Mat src(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1, ptrGrabResult->GetBuffer());

                // 이미지 데이터에 접근.  // 접근~캡쳐된 이미지를 표시 하는 부분에서 grab 만져보기
                cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl;
                cout << "time : " << time_watch << endl << endl;

                //이미지 받아오고 src형변환 후 imshow
                //gray로 변환해주었기때문에 CV_8UC3으로 변경 dvd
                //src = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)ptrGrabResult->GetBuffer());
                
                // 왜 ?
                formatConverter.Convert(pylonImage, ptrGrabResult);
                Mat src = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)pylonImage.GetBuffer());


                //for (int i = 0; i < 6; i++)  //여러 번 실행하니 최소값과 최대값이 자꾸 바뀌는 상황 발생 - 주석처리
                //{
                    Mat img_out;
                    src.copyTo(img_out);

                    //int Matching_method = i;
                    /*
                    0: TM_SQDIFF
                    1: TM_SQDIFF NORMED
                    2: TM CCORR
                    3: TM CCORR NORMED
                    4: TM COEFF
                    5: TM COEFF NORMED";
                    */

                    start = clock();
                    //for (int i = 0; i < 2; i++)
                    
                        // 원본 이미지에서 탬픞릿 이미지와 일치하는 영역을 찾는 알고리즘
                        matchTemplate(src, templ, result, i);

                        // normalize를 이용해서 이미지 정규화, 필터의 종류, 0~1까지 분포
                        normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

                        // 주어진 행력의 최소값, 최대값을 찾는 함수로 최소값, 최대값이 있는 좌표정보도 함께 알아낼 수 있음
                        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

                        //처음은 minLoc나오게 출력
                        // 그 뒤는 maxLoc나오게 출력

                        if (i == 0)
                        {
                            matchLoc = minLoc;

                            rectangle(img_out, matchLoc, Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows), Scalar(0, 0, 255), 1);

                            //cvtColor 함수를 이용하여 결과사진을 gray로 변경  //코드 간단하게 할 수 있으면 수정
                            cvtColor(result, result, COLOR_GRAY2BGR);

                            i = 1;
                        }
                        else
                        {
                            matchLoc = maxLoc;

                            rectangle(img_out, matchLoc, Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows), Scalar(0, 0, 255), 1);

                            i = 0;
                        }

                        //좌표로 설정된 사각형 색 설정같음  BGR    
                        circle(result, matchLoc, 3, Scalar(0, 0, 255), 1);

                        // imshow 이미지 출력 함수
                        imshow("src", img_out);
                        imshow("templ", templ);
                        imshow("result", result);  //result쪽에서 문제가 발생했으니 result를 변경하면 1개가 뜨나 ?
                        //또는 imshow에서 추가나 변경, 삭제 해보기
                    
                    waitKey(1); //waitKey()로 바꿔보기
                

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
        exitCode = 1;
    }

    // 종료 전에 대기를 비활성화하려면 다음 두 줄을 주석 처리.
    cerr << endl << "Press enter to exit." << endl;
    while (cin.get() != '\n');

    // 모든 pylon 리소스를 해제.
    PylonTerminate();

    return exitCode;
}