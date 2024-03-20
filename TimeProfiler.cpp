#include<iostream>
#include<fstream>
#include<string>
#include<chrono>
#include<algorithm>
#include<thread>
#include<cmath>

//该宏定义用于创建独一无二的timer（为了避免profiling作用域的重复所以与行绑定）
//This macro definition is used to create a unique timer (bound to a row to avoid duplication of the profiling scope)
#define PROFILE_BASE(FuncName) ProfileTimer timer##__LINE__(FuncName)
//该宏定义用于将函数名称（真名）直接绑定（这样就不用自主输入函数名称了）
//This macro definition is used to bind the function name (real name) directly (so that you don't have to enter the function name yourself)
#define PROFILE_ACT() PROFILE_BASE(__FUNCSIG__)

struct ProfileResult
{
	std::string FuncName;
	long long Start, End;
	uint32_t TempThread;
};

struct ProfileSession
{
	std::string FuncName;
};

class Profiler
{
private:
	ProfileSession* CurrentSession;
	std::ofstream OutputStream;
	uint32_t ProfileCount;
public:
	Profiler():CurrentSession(nullptr),ProfileCount(0){}

	void BeginSession(const std::string& FuncName, const std::string& filepath = "Results.json")
	{
		OutputStream.open(filepath);
		WriteHeader();
		CurrentSession = new ProfileSession();
	}

	void EndSession()
	{
		WriteFooter();
		OutputStream.close();
		delete CurrentSession;
		CurrentSession = nullptr;
		ProfileCount = 0;
	}

	void WriteProfile(const ProfileResult& result)
	{
		if (ProfileCount++ > 0) 
		{
			OutputStream << ",";
		}
		std::string FuncName = result.FuncName;
		//将所有双引号字符替换为单引号（使用到了转义符\下同）
		std::replace(FuncName.begin(), FuncName.end(), '"', '\'');
		//构建json结构（键值对）：这一键值对是根据Chrome的Trace Viewer Format构建的
		OutputStream << "{";
		//事件分类category
		OutputStream << "\"cat\":\"Process Function\",";
		//历程duration
		OutputStream << "\"dur\":" << (result.End - result.Start) << ',';
		//事件名name
		OutputStream << "\"name\":\"" << FuncName <<"\",";
		//阶段phase
		OutputStream << "\"ph\":\"X\",";
		//进程名pid
		OutputStream << "\"pid\":\"Profiling\",";
		//线程名tid
		OutputStream << "\"tid\":" << result.TempThread << ",";
		//时间戳ts
		OutputStream << "\"ts\":" << result.Start;
		OutputStream << "}";
		//使用flush不等待输出缓冲直接将输出内容写入
		OutputStream.flush();
	}

	void WriteHeader()
	{
		OutputStream << "{\"ohterData\":{\"msg\":\"This is a visual version of profiling.\"},\"traceEvents\":[";
		OutputStream.flush();
	}

	void WriteFooter()
	{
		OutputStream << "]}";
		OutputStream.flush();
	}

	static Profiler& Get()
	{
		static Profiler* instance = new Profiler();
		return *instance;
	}
};

class ProfileTimer
{
private:
	const char* FuncName;
	std::chrono::time_point<std::chrono::steady_clock> StartPoint;
	bool StopFlag;
public:
	ProfileTimer(const char* FuncName) : FuncName(FuncName), StopFlag(false) 
	{ 
		StartPoint = std::chrono::high_resolution_clock::now(); 
	}
	void Stop()
	{
		auto EndPoint = std::chrono::high_resolution_clock::now();
		long long start = std::chrono::time_point_cast<std::chrono::microseconds>(StartPoint).time_since_epoch().count();
		long long end = std::chrono::time_point_cast<std::chrono::microseconds>(EndPoint).time_since_epoch().count();
		uint32_t TempThread = std::hash<std::thread::id>{}(std::this_thread::get_id());
		Profiler::Get().WriteProfile({ FuncName, start, end, TempThread });
		StopFlag = true;
	}
	~ProfileTimer() 
	{ 
		if (!StopFlag) 
		{ 
			Stop(); 
		} 
	}
};

//示范 Demo
void Foo_One()
{
	PROFILE_ACT();
	for (int i = 0; i < 500; i++)
	{
		std::cout << "Count#" << i << std::endl;
	}
}

void Foo_Two()
{
	PROFILE_ACT();
	for (int i = 0; i < 1000; i++)
	{
		std::cout << "Count#" << i << std::endl;
	}
}

void BenchMark()
{
	PROFILE_ACT();
	Foo_One();
	Foo_Two();
	Foo_One();
	Foo_Two();
}

/*
int main()
{
	Profiler::Get().BeginSession("Profile");
	BenchMark();
	Profiler::Get().EndSession();
	std::cin.get();
}
*/
