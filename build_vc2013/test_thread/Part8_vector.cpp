// from http://thispointer.com/c11-multithreading-part-8-stdfuture-stdpromise-and-returning-values-from-thread/
// std::future and std::promise

#include <iostream>
#include <thread>
#include <future>
#include <memory>
 
void initiazer_vec(int i, std::promise<int> &p)
{
    std::cout<<"Inside Thread"<<std::endl;    p.set_value(i);
}
 
int main()
{
    std::vector< std::future<int> > futureObjs;
    std::vector< std::thread > threads;
		
	int num_threads = 8;

	for (int i=0;i<num_threads;i++)
	{
		std::promise<int> promiseObj;
		futureObjs.push_back(promiseObj.get_future());
		threads.push_back(std::thread(initiazer_vec,i,std::move(promiseObj)));
	    threads[i].detach();
	}
	/*
	for (int i=0;i<num_threads;i++)
	{
		threads[i].detach();
	}
	*/
	for (int i=0;i<num_threads;i++)
	{
		    std::cout<<futureObjs[i].get()<<std::endl;
	}
    return 0;
}