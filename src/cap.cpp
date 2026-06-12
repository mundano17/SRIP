#include "ble.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <opencv2/opencv.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

struct RGB {
  uint8_t r, g, b;
};

// Sample box half-size in pixels — tweak this to make the dot bigger/smaller
static const int SAMPLE_RADIUS = 20;

std::string rgbToHex(const RGB &c) {
  std::stringstream ss;
  ss << "#" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
     << (int)c.r << std::setw(2) << std::setfill('0') << (int)c.g
     << std::setw(2) << std::setfill('0') << (int)c.b;
  return ss.str();
}

// Sample average color from a small box around (cx, cy)
RGB sampleAt(const cv::Mat &frame, int cx, int cy, int radius) {
  int x1 = std::max(0, cx - radius);
  int y1 = std::max(0, cy - radius);
  int x2 = std::min(frame.cols - 1, cx + radius);
  int y2 = std::min(frame.rows - 1, cy + radius);

  cv::Rect box(x1, y1, x2 - x1, y2 - y1);
  cv::Mat region = frame(box).clone();

  cv::Scalar avg = cv::mean(region);
  return RGB{static_cast<uint8_t>(avg[2]), static_cast<uint8_t>(avg[1]),
             static_cast<uint8_t>(avg[0])};
}

void postColorAsync(const std::string &hex, int r, int g, int b) {
  std::thread([hex, r, g, b]() {
    std::string body = "{\"hex\":\"" + hex +
                       "\","
                       "\"r\":" +
                       std::to_string(r) +
                       ","
                       "\"g\":" +
                       std::to_string(g) +
                       ","
                       "\"b\":" +
                       std::to_string(b) + "}";

    std::string req = "POST /color HTTP/1.0\r\n"
                      "Host: 127.0.0.1:3000\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: " +
                      std::to_string(body.size()) +
                      "\r\n"
                      "Connection: close\r\n"
                      "\r\n" +
                      body;

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
      std::cerr << "\n[socket] failed\n";
      return;
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3000);
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (::connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      std::cerr << "\n[socket] connect failed\n";
      ::close(fd);
      return;
    }

    ::send(fd, req.c_str(), req.size(), 0);
    char buf[512];
    while (::recv(fd, buf, sizeof(buf), 0) > 0) {
    }
    ::close(fd);
    std::cout << " sent!\n";
  }).detach();
}

cv::VideoCapture openCamera(int index) {
  cv::VideoCapture cap(index);
  if (!cap.isOpened()) {
    std::cerr << "no camera\n";
  }

  // Warmup — discard first few frames
  cv::Mat tmp;
  for (int i = 0; i < 5; ++i)
    cap >> tmp;

  return cap;
}

int *setupWindow(const std::string &name) {
  cv::namedWindow(name, cv::WINDOW_AUTOSIZE);

  int *ptr = new int[2]{-1, -1};
  cv::setMouseCallback(
      name,
      [](int event, int x, int y, int, void *ud) {
        if (event == cv::EVENT_MOUSEMOVE) {
          ((int *)ud)[0] = x;
          ((int *)ud)[1] = y;
        }
      },
      ptr);

  return ptr;
}

cv::Point clampSamplePoint(const cv::Mat &frame, int rawX, int rawY,
                           int radius) {
  int cx = (rawX < 0) ? frame.cols / 2 : rawX;
  int cy = (rawY < 0) ? frame.rows / 2 : rawY;
  cx = std::max(radius, std::min(frame.cols - radius - 1, cx));
  cy = std::max(radius, std::min(frame.rows - radius - 1, cy));
  return {cx, cy};
}

void drawSampleBox(cv::Mat &frame, cv::Point center, int radius) {
  cv::rectangle(frame, cv::Point(center.x - radius, center.y - radius),
                cv::Point(center.x + radius, center.y + radius),
                cv::Scalar(0, 255, 0), 2);

  cv::drawMarker(frame, center, cv::Scalar(0, 255, 0), cv::MARKER_CROSS, 10, 1);
}

void drawHUD(cv::Mat &frame, const RGB &color, const std::string &hex) {
  cv::putText(frame, "HEX: " + hex, cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX,
              0.8, cv::Scalar(255, 255, 255), 2);

  cv::putText(frame,
              "RGB: (" + std::to_string(color.r) + "," +
                  std::to_string(color.g) + "," + std::to_string(color.b) + ")",
              cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 0.7,
              cv::Scalar(255, 255, 255), 2);

  cv::putText(frame, "S = send  Q = quit", cv::Point(20, frame.rows - 15),
              cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(180, 180, 180), 1);
}

void drawColorPreview(cv::Mat &frame, const RGB &color) {
  const int pw = 80;
  cv::Mat preview = frame(cv::Rect(frame.cols - pw - 10, 10, pw, pw));
  preview.setTo(cv::Scalar(color.b, color.g, color.r));
  cv::rectangle(frame, cv::Rect(frame.cols - pw - 10, 10, pw, pw),
                cv::Scalar(200, 200, 200), 1);
}

void printColorToConsole(const RGB &color, const std::string &hex) {
  std::cout << "\rHEX: " << hex << "  RGB(" << (int)color.r << ","
            << (int)color.g << "," << (int)color.b << ")      " << std::flush;
}

void runLoop(cv::VideoCapture &cap, int *ptr, BLEColorClient &ble) {
  cv::Mat frame;

  while (true) {
    cap >> frame;
    if (frame.empty() || frame.type() != CV_8UC3)
      continue;
    if (!frame.isContinuous())
      frame = frame.clone();

    cv::Point center = clampSamplePoint(frame, ptr[0], ptr[1], SAMPLE_RADIUS);

    RGB color = sampleAt(frame, center.x, center.y, SAMPLE_RADIUS);
    std::string hex = rgbToHex(color);

    drawSampleBox(frame, center, SAMPLE_RADIUS);
    drawHUD(frame, color, hex);
    drawColorPreview(frame, color);
    printColorToConsole(color, hex);

    cv::imshow("Camera", frame);

    char key = static_cast<char>(cv::waitKey(1));

    if (key == 's' || key == 'S') {
      std::cout << "\nSending " << hex << " ...";
      postColorAsync(hex, color.r, color.g, color.b);
      ble.sendColor(color.r, color.g, color.b);
    }
    if (key == 'q' || key == 'Q')
      break;
  }
}

int main() {
  BLEColorClient ble;
  if (!ble.initialize()) {
    return -1;
  }
  ble.connect();
  cv::VideoCapture cap = openCamera(0);
  if (!cap.isOpened())
    return -1;

  std::cout << "\nControls\n";
  std::cout << "S - Send color under pointer to frontend\n";
  std::cout << "Q - Quit\n\n";

  int *ptr = setupWindow("Camera");

  runLoop(cap, ptr, ble);

  delete[] ptr;
  cap.release();
  cv::destroyAllWindows();
  return 0;
}
