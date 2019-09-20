#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include <future>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <vector>
#include <string>

#include "opencog.grpc.pb.h"
#include "OpencogSNETServiceFactory.h"
#include "OpencogSNETService.h"
#include "GuileSessionManager.h"

// seconds
#define ASYNCHRONOUS_API_UPDATE_INTERVAL ((unsigned int) 60)

#define ASYNCHRONOUS_API_OUTPUT_URL "http://54.203.198.53:7000/ServiceAsynchronousOutput/opencog/"
#define ASYNCHRONOUS_API_OUTPUT_DIR "/home/admin/storage/ServiceAsynchronousOutput/opencog/"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;

using namespace opencog_services;
using namespace std;

static string baseOutputURL;
static string baseOutputDir;
static GuileSessionManager *gpSessionManager;

static void initGuileSessionManager(const char *execPath)
{
	char path_save[PATH_MAX];
	char abs_exe_path[PATH_MAX];
	char *p;
	strncpy(abs_exe_path, execPath, strlen(execPath));

	if (!(p = strrchr(abs_exe_path, '/'))) {
		getcwd(abs_exe_path, sizeof(abs_exe_path));
	} else {
		*p = '\0';
		getcwd(path_save, sizeof(path_save));
		
		if(int chdir_err = chdir(abs_exe_path)) {
			printf("Failed to change to directory %s - %d\n", abs_exe_path, chdir_err);
		}
		getcwd(abs_exe_path, sizeof(abs_exe_path));
		chdir(path_save);
	}
	
	// initialize guile session manager and set the guile session absolute executable path
	gpSessionManager = new GuileSessionManager(abs_exe_path);
}

static void handleSignal(int sig)
{
	bool free_guile_session_manager = false;

	switch (sig) {
		case SIGINT:
			free_guile_session_manager = true;
			break;
		case SIGKILL:
			free_guile_session_manager = true;
			break;
		case SIGTERM:
			free_guile_session_manager = true;
			break;
		case SIGPWR:
			free_guile_session_manager = true;
			break;
	}

	if (free_guile_session_manager) {
		/* guarantee that the created sessions will be deleted for any of the registered signals.
		 * otherwise the session manager will kill them during its next startup
		 */
		delete gpSessionManager;
	}

	// exit the program by returning the received signal
	exit(sig);
}

class OpencogService final : public OpencogServices::Service
{
public:
    explicit OpencogService() {
        baseOutputURL = ASYNCHRONOUS_API_OUTPUT_URL;
        baseOutputDir = ASYNCHRONOUS_API_OUTPUT_DIR;
    }

    Status Execute(ServerContext* context, const Command* input, CommandOutput* output) override {
        int status = execService(context, input, output);
		if (status == 0) {
			return Status::OK;
		}
		else {
			string value = output->output();
			Status(StatusCode::UNKNOWN,value);
		}
    }

    Status AsynchronousTask(ServerContext* context, const Command* input, CommandOutput* output) override {
		//TODO::reimplement with current architecture
        return Status::OK;
    }

	int execService(ServerContext* context, const Command* args, CommandOutput* output) {
		// set basic call parameters
		string service_name = args->input()[0];

		OpencogSNETService *opencogService = OpencogSNETServiceFactory::factory(service_name);
		
		int status;
		
		if (opencogService == NULL) {
			output->set_output(service_name + ": Opencog service not found");
			return 1;
		} else {
			// set process based guile session manager for this service
			opencogService->setGuileSessionManager(gpSessionManager);

			// prepare parameters
			vector<string> service_args;

			// push all other arguments
			for(int arg = 1; arg < args->input_size(); arg++) {
				service_args.push_back(args->input()[arg]);
			}

			// service response
			string out;

			// hold the response in value and return it to the server to rely 
			// this response to the clients in a meaninful manner.
			status = opencogService->execute(out, service_args);
			
			// free mem
			delete opencogService;

			if (status != 0) {
				output->set_output("Error in " + service_name + ": " + out);
				return 1;
			} else {
				output->set_output(out);
				return 0;
			}
		}
	}

	void threadJobManager(ServerContext* context, const Command* input, const string &fname)
	{
		CommandOutput command_output;

		auto future = std::async(std::launch::async, &OpencogService::execService, this, context, input, &command_output);

		unsigned int count = ASYNCHRONOUS_API_UPDATE_INTERVAL;
		while (true) {
			auto threadStatus = future.wait_for(std::chrono::seconds(0));
			if (threadStatus == std::future_status::ready) {
				break;
			} else {
				if (count == ASYNCHRONOUS_API_UPDATE_INTERVAL) {
					count = 0;
					auto now = std::chrono::system_clock::now();
					auto now_c = std::chrono::system_clock::to_time_t(now);
					//string s = "Work in progress. Last update: " + std::time_put(std::localtime(&now_c), "%Y-%m-%d %X") + "\n";
					char buffer[256];
					strftime(buffer, sizeof(buffer)," Work in progress. Last update: %d-%m-%Y %H:%M:%S\n", std::localtime(&now_c));
					// TODO : this need to be improved to use a more robust method
					FILE *f = fopen(fname.c_str(), "w");
					fputs(buffer, f);
					fclose(f);
				}
				count++;
				sleep(1); // 1 second
			}
		}

		string s = "Service finished. Output:\n" + command_output.output();
		FILE *f = fopen(fname.c_str(), "w");
		fputs(s.c_str(), f);
		fclose(f);
	}
};

static void RunServer() {
	std::string server_address;
    
	if (char * server_port = getenv("OPENCOG_SERVER_PORT")) {
		std::string port(server_port);
	    server_address = "0.0.0.0:" + port ;
	}
	else {
		printf("Warning: Using default OPENCOG_SERVER_PORT: 7032\n");
	    server_address = "0.0.0.0:7032";
	}
	
	OpencogService service;
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    initGuileSessionManager(argv[0]);
    signal(SIGINT, handleSignal);
    RunServer();
    return 0;
}
