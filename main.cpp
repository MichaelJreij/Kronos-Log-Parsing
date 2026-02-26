// Copyright 2023 <Michael Jreij>"
#include <iostream>
#include <string>
#include <fstream>
#include <regex>
#include <iomanip>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

struct BootInfo {
    std::string startLine;
    int startLineNumber;
    std::string completeLine;
    int completeLineNumber;
    boost::posix_time::ptime startTime;
    boost::posix_time::ptime completeTime;
    int bootTimeSeconds;
};

void displayBootInfo(std::ofstream& output_file_stream,
const BootInfo& bootInfo, const std::string& logFileName) {
    output_file_stream << "=== Device boot ===" << std::endl;

    boost::gregorian::date bootDate = bootInfo.startTime.date();
    std::string formattedMonth =
    boost::gregorian::to_iso_string(bootDate).substr(4, 2);

    output_file_stream << bootInfo.startLineNumber
    << "(" << logFileName << "): "
    << bootInfo.startTime.date().year() << "-" << formattedMonth << "-" <<
    std::setw(2) << std::setfill('0') << bootInfo.startTime.date().day()
                       << " " << bootInfo.startTime.time_of_day()
                       << " Boot Start" << std::endl;

    if (!bootInfo.completeLine.empty()) {
        output_file_stream << bootInfo.completeLineNumber << "(" << logFileName
        << "): " << bootInfo.completeTime.date().year() << "-" << formattedMonth
        << "-" << std::setw(2) << std::setfill('0')
        << bootInfo.completeTime.date().day()
                           << " " << bootInfo.completeTime.time_of_day()
                           << " Boot Completed" << std::endl;
        output_file_stream << "\tBoot Time: " <<
        bootInfo.bootTimeSeconds << "ms" << std::endl;
    } else {
        output_file_stream << "**** Incomplete boot ****" << std::endl;
    }

    output_file_stream << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <log_file>" << std::endl;
        return 1;
    }

    std::string log_file = argv[1];

    // Append ".rpt" to the file name
    std::string output_file = log_file + ".rpt";

    try {
        std::ifstream input_file(log_file);

        if (!input_file.is_open()) {
            std::cerr << "Error: Unable to open input file "
            << log_file << std::endl;
            return 1;
        }

        std::ofstream output_file_stream(output_file);

        if (!output_file_stream.is_open()) {
            std::cerr << "Error: Unable to open output file "
            << output_file << std::endl;
            return 1;
        }

        std::regex pattern1(R"(\(log\.c\.166\) server started)");
        std::regex pattern2(R"(oejs\.AbstractConnector:Started SelectChannelConnector@0\.0\.0\.0:9080)");

        BootInfo currentBoot;
        bool inBoot = false;

        std::string line;
        int lineNum = 0;
        while (std::getline(input_file, line)) {
            lineNum++;

            if (std::regex_search(line, pattern1)) {
                if (inBoot) {
                    currentBoot.completeLineNumber =
                    currentBoot.completeLine.empty() ? lineNum : lineNum - 1;
                    displayBootInfo(output_file_stream, currentBoot, log_file);
                }

                currentBoot.startLine = line;
                currentBoot.startLineNumber = lineNum;
                currentBoot.completeLine.clear();
                currentBoot.startTime =
                boost::posix_time::time_from_string(line.substr(0, 19));
                currentBoot.bootTimeSeconds = 0;
                inBoot = true;
            } else if (std::regex_search(line, pattern2)) {
                if (inBoot) {
                    currentBoot.completeLine = line;
                    currentBoot.completeTime =
                    boost::posix_time::time_from_string(line.substr(0, 19));

                    boost::posix_time::time_duration bootDuration =
                    currentBoot.completeTime - currentBoot.startTime;
                    currentBoot.bootTimeSeconds =
                    static_cast<int>(bootDuration.total_milliseconds());

                    currentBoot.completeLineNumber = lineNum;
                    displayBootInfo(output_file_stream, currentBoot, log_file);

                    currentBoot = BootInfo();
                    inBoot = false;
                }
            }
        }

        if (inBoot) {
            currentBoot.completeLineNumber =
            currentBoot.completeLine.empty() ? lineNum : lineNum - 1;
            displayBootInfo(output_file_stream, currentBoot, log_file);
        }

        std::cout << "Report created successfully: "
        << output_file << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}


// ./ps7 device5_intouch.log
