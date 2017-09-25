// from http://thispointer.com/c11-multithreading-part-9-stdasync-tutorial-example/
// std::async
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <future>
#include <vector>
 
using namespace std::chrono;
 
std::string vecfetchDataFromDB(std::string recvdData)
{
	// Make sure that function takes 5 seconds to complete
	//std::this_thread::sleep_for(seconds(5));
	double x = 0;
	for (size_t i=0;i<100000000; i++)
		x=sin(i*0.4)+cos(i*0.5);
 
	//Do stuff like creating DB Connection and fetching Data
	return "DB_" + recvdData;
}
 
std::string vecfetchDataFromFile(std::string recvdData)
{
	// Make sure that function takes 5 seconds to complete
	std::this_thread::sleep_for(seconds(5));
 
	//Do stuff like fetching Data File
	return "File_" + recvdData;
}
 
int main()
{
	std::vector< std::future<std::string> > results;
	std::vector< std::string > dbData;	
	int num_threads=12;

	system_clock::time_point start = system_clock::now();
	for (int i=0; i<num_threads; i++)
	{
//		std::cout << "spawn " << i << std::endl;
		results.push_back( std::async(std::launch::async, vecfetchDataFromDB, "Data" + std::to_string(i)));
	}
	//Fetch Data from DB
	// Will block till data is available in future<std::string> object.
	for (int i=0; i<num_threads; i++)
	{
		dbData.push_back( results[i].get());
//		std::cout << "get " << i << std::endl;
	}
	// Get End Time
	auto end = system_clock::now();

	std::cout << "System supports " << std::thread::hardware_concurrency() << " separate threads." << std::endl;

	auto diff = duration_cast < std::chrono::seconds > (end - start).count();
	std::cout << "Total Time Taken with " << num_threads << " async threads = " << diff << " Seconds" << std::endl;

	//Combine The Data
	std::string data;
	for (int i=0; i<num_threads; i++)
	{
		data += dbData[i] + " ";
	}
	//Printing the combined Data
	std::cout << "Data = " << data << std::endl;
	// sequential call comparison
	dbData.clear();

	start = system_clock::now();
	for (int i=0; i<num_threads; i++)
	{
		dbData.push_back(vecfetchDataFromDB("sData" + std::to_string(i)));
	}
	end = system_clock::now();

	diff = duration_cast < std::chrono::seconds > (end - start).count();
	std::cout << "Total Time Taken with " << num_threads << " sequential calls = " << diff << " Seconds" << std::endl;

	data = "";
	for (int i=0; i<num_threads; i++)
	{
		data += dbData[i] + " ";
	}
 	std::cout << "Data = " << data << std::endl;
	
	return 0;
}