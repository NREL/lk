// from http://thispointer.com/c11-multithreading-part-9-stdasync-tutorial-example/
// std::async
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <future>
 
using namespace std::chrono;
 
std::string fetchDataFromDB(std::string recvdData)
{
	// Make sure that function takes 5 seconds to complete
	std::this_thread::sleep_for(seconds(5));
 
	//Do stuff like creating DB Connection and fetching Data
	return "DB_" + recvdData;
}
 
std::string fetchDataFromFile(std::string recvdData)
{
	// Make sure that function takes 5 seconds to complete
	std::this_thread::sleep_for(seconds(5));
 
	//Do stuff like fetching Data File
	return "File_" + recvdData;
}
 
int main()
{
	// Get Start Time
	system_clock::time_point start = system_clock::now();
 
	std::cout << "spawn 1 " << std::endl;
	std::future<std::string> resultFromDB = std::async(std::launch::async, fetchDataFromDB, "Data");
	std::cout << "spawn 2 " << std::endl;
 	std::future<std::string> resultFromDB2 = std::async(std::launch::async, fetchDataFromDB, "Data2");
 
 
	//Fetch Data from File
//	std::string fileData = fetchDataFromFile("Data");
 
	//Fetch Data from DB
	// Will block till data is available in future<std::string> object.
	std::string dbData = resultFromDB.get();
	std::cout << "get 1 " << std::endl;
	std::string dbData2 = resultFromDB2.get();
	std::cout << "get 2 " << std::endl;
 
	// Get End Time
	auto end = system_clock::now();
 
	auto diff = duration_cast < std::chrono::seconds > (end - start).count();
	std::cout << "Total Time Taken = " << diff << " Seconds" << std::endl;
 
	//Combine The Data
	std::string data = dbData + " :: " + dbData2 ;//+ " :: " + fileData;
 
	//Printing the combined Data
	std::cout << "Data = " << data << std::endl;
 
	return 0;
}