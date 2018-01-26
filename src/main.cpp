#include "sat/solver/selection/VSIDS.h"
#include "sat/solver/NS.h"
#include <iostream>

#include <chrono>
#include <random>

void run(const char *fn) {
	// sequential
	//sat::selection::Sequential h;

	// random
	//std::random_device rd;
	//sat::selection::Random h(rd());

	// VSIDS
	sat::selection::VSIDS h;

	sat::NS ns(h);
	try {
		sat::addCNF(ns, fn);
	} catch (...) {
		std::cout << "not found\n\n";
		return;
	}
	
	auto ts = std::chrono::system_clock::now();
	auto res = ns.solve();
	auto te = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = te-ts;
	std::cout << " (solved in " << elapsed_seconds.count() << "s) ";

	if (res) std::cout << "SAT";
	else std::cout << "UNSAT";

	std::cout << "\n\n";
}

int main(int argc, char *argv[]) {
	if (argc != 2) puts("usage: nsat <in>");
	else run(argv[1]);
	return 0;
}
