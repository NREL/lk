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
	//std::this_thread::sleep_for(seconds(5));
	double x = 0;
	for (size_t i=0;i<100000000; i++)
		x=sin(i*0.4)+cos(i*0.5);
 
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
 
int main_9()
{
	// Get Start Time
	system_clock::time_point start = system_clock::now();
	std::cout << "spawn 1 " << std::endl;
	std::future<std::string> resultFromDB1 = std::async(std::launch::async, fetchDataFromDB, "Data1");
	std::cout << "spawn 2 " << std::endl;
 	std::future<std::string> resultFromDB2 = std::async(std::launch::async, fetchDataFromDB, "Data2");
	std::cout << "spawn 3 " << std::endl;
	std::future<std::string> resultFromDB3 = std::async(std::launch::async, fetchDataFromDB, "Data3");
	std::cout << "spawn 4 " << std::endl;
 	std::future<std::string> resultFromDB4 = std::async(std::launch::async, fetchDataFromDB, "Data4");
	//Fetch Data from DB
	// Will block till data is available in future<std::string> object.
	std::string dbData1 = resultFromDB1.get();
	std::cout << "get 1 " << std::endl;
	std::string dbData2 = resultFromDB2.get();
	std::cout << "get 2 " << std::endl;
	std::string dbData3 = resultFromDB3.get();
	std::cout << "get 3 " << std::endl;
	std::string dbData4 = resultFromDB4.get();
	std::cout << "get 4 " << std::endl;
	// Get End Time
	auto end = system_clock::now();
	auto diff = duration_cast < std::chrono::seconds > (end - start).count();
	std::cout << "Total Time Taken = " << diff << " Seconds" << std::endl;
	//Combine The Data
	std::string data = dbData1 + " :: " + dbData2  + " :: " +  dbData3 + " :: " + dbData4 ;//+ " :: " + fileData;
	//Printing the combined Data
	std::cout << "Data = " << data << std::endl;
	// sequential call comparison
	start = system_clock::now();
	dbData1 = fetchDataFromDB("sData1");
	dbData2 = fetchDataFromDB("sData2");
	dbData3 = fetchDataFromDB("sData3");
	dbData4 = fetchDataFromDB("sData4");
	end = system_clock::now();
	diff = duration_cast < std::chrono::seconds > (end - start).count();
	std::cout << "Sequntial Total Time Taken = " << diff << " Seconds" << std::endl;
	data = dbData1 + " :: " + dbData2  + " :: " +  dbData3 + " :: " + dbData4 ;//+ " :: " + fileData;
 	std::cout << "Data = " << data << std::endl;
	return 0;
}