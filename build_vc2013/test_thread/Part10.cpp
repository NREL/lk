// From http://thispointer.com/c11-multithreading-part-10-packaged_task-example-and-tutorial/
// packaged_task

#include <iostream>
#include <thread>
#include <future>
#include <string>
 
// Fetch some data from DB
std::string getDataFromDB( std::string token)
{
	// Do some stuff to fetch the data
	std::string data = "Data fetched from DB by Filter :: " + token;
	return data;
}


int main()
{
 
	// Create a packaged_task<> that encapsulated the callback i.e. a function
	std::packaged_task<std::string (std::string)> task(getDataFromDB);
 
	// Fetch the associated future<> from packaged_task<>
	std::future<std::string> result = task.get_future();
 
	// Pass the packaged_task to thread to run asynchronously
	std::thread th(std::move(task), "Arg");
 
	// Join the thread. Its blocking and returns when thread is finished.
	th.join();
 
	// Fetch the result of packaged_task<> i.e. value returned by getDataFromDB()
	std::string data =  result.get();
 
	std::cout <<  data << std::endl;
 
	// list hardware dependency if not implementing a thread pool
	int Num_Threads =  std::thread::hardware_concurrency();

	std::cout <<  "Your hardware supports " << Num_Threads << " threads." << std::endl;
 

	return 0;
}
