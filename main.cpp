#include "MyFFmpeg.h"
#include <iostream>
#include <chrono>
#include <pybind11/pybind11.h>

namespace py = pybind11;


int main() {

	auto start = std::chrono::high_resolution_clock::now();
	//const std::string url = "D:\\Downloads\\669_41010020031825864_4_87d3dc34-625a-4b5d-a103-c5b9c40f3206.flv";
	const std::string url = "D:\\Downloads\\0430.flv";
	//const std::string url = "http://live3.wopuwulian.com/mp/5EQ1i693Co578s43q95d35f3L993Eu.flv?auth_key=1691151598-0-0-467983dc01f8875fee4c36a0d1c95c9d";
	MyFFmpeg f(url, 0.4, 15, 30, true); // 0~1
	int ret = f.initialize();
	if (ret != 3100) {
		std::cout << "error: " << ret << std::endl;
		return -1;
	}
	int i = 0;
	py::list pyret;
	while (true) {
		ret = f.decode();
		if (ret == 3121 || ret == 3144) {
			std::cout << "ret: " << ret << std::endl;
			break;
		}
		pyret = f.frames();
		std::cout << "pyret" << i << ": " << pyret.size() << std::endl;
		//std::cout << "pyret" << i << "array_t: " << pyret[pyret->size() - 1].cast<py::array_t<double>>().mutable_data()[0] << std::endl;
		i++;

		if (i > 100) {
			break;
		}
	}

	pyret = f.frames();
	std::cout << "pyret" << i << ": " << pyret.size() << std::endl;

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
	std::cout << "spend_time:" << duration.count() << " ms" << std::endl;

}
