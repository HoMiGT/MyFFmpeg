#include "MyFFmpeg.h"
#include <iostream>
#include <chrono>


int main() {

	auto start = std::chrono::high_resolution_clock::now();
	const std::string url = "D:\\Downloads\\669_41010020031825864_4_87d3dc34-625a-4b5d-a103-c5b9c40f3206.flv";
	MyFFmpeg f(url, 0.4);
	int ret = f.initialize();
	if (ret != 3100) {
		std::cout << "error: " << ret << std::endl;
		return -1;
	}
	int i = 0;
	while (true) {
		ret = f.frame(i);
		if (ret == 3121 || ret == 3144) {
			std::cout << "ret: " << ret << std::endl;
			break;
		}
		i++;
	}
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
	std::cout << "spend_time:" << duration.count() << std::endl;


}