#include <iostream>
#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/client_context.h>

#include "dpf_pir.grpc.pb.h"
#include "dpf.h"
#include "hashdatastore.h"
#include <immintrin.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace std;
using dpfpir::Answer;
using dpfpir::DPFPIRInterface;
using dpfpir::FuncKey;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientWriter;
using grpc::Status;

class DpfPirClient
{
public:
    string client_id;
    size_t query_index;

private:
    std::unique_ptr<DPFPIRInterface::Stub> stub_;
    string serverAddr;

public:
    explicit DpfPirClient(std::shared_ptr<Channel> channel, string &client_id, size_t query_index, string serverAddr) : stub_(DPFPIRInterface::NewStub(channel)), client_id(client_id), query_index(query_index), serverAddr(serverAddr){};

    hashdatastore::hash_type DpfPir(std::vector<uint8_t> funckey)
    {
        FuncKey request;
        Answer reply;
        ClientContext context;
        context.AddMetadata("client_id", this->client_id);

        /* set funckey */
        string key_str(funckey.begin(), funckey.end());
        request.set_funckey(key_str);

        Status status = stub_->DpfPir(&context, request, &reply);
        if (status.ok())
        {

            std::cout << "[" << this->client_id << "][" << this->serverAddr << "] "
                      << "2.Receive PIR result." << std::endl;
            return stringToM256i(reply.answer());
        }
        else
        {
            std::cout << "RPC failed" << std::endl;
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return __m256i();
        }
    }

private:
    // Convert string to __m256i
    __m256i stringToM256i(const std::string &str)
    {
        __m256i result;
        alignas(32) uint8_t buffer[32];

        // Ensure the string is large enough to fill the buffer
        if (str.size() >= sizeof(buffer))
        {
            // Copy characters from the string to the buffer
            for (size_t i = 0; i < sizeof(buffer); ++i)
            {
                buffer[i] = static_cast<uint8_t>(str[i]);
            }

            // Load the buffer into the __m256i variable
            result = _mm256_load_si256((__m256i *)buffer);
        }
        else
        {
            // Handle the case where the string is too short
            std::cerr << "Error: String is too short to convert to __m256i." << std::endl;
            // You might want to handle this differently based on your requirements
        }

        return result;
    }
};

int main(int argc, char *argv[])
{
    /* 219.245.186.51 */
    string Addr = "localhost";
    string serverAddr0 = Addr + ":50050";
    string serverAddr1 = Addr + ":50051";

#pragma region args
    /* args */
    string client_id;
    size_t query_index;
    try
    {
        // def options
        po::options_description desc("Allowed options");
        desc.add_options()("help,h", "Produce help message")("id", po::value<std::string>()->required(), "client id (string)")("q", po::value<size_t>()->required(), "query index (size_t)");

        // parse params
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        // result
        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            return 0;
        }

        if (vm.count("id"))
        {
            client_id = vm["id"].as<std::string>();
            // std::cout << "Input file: " << vm["input-file"].as<std::string>() << std::endl;
        }
        if (vm.count("q"))
        {
            query_index = vm["q"].as<size_t>();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
#pragma endregion args

    /* RPC */
    DpfPirClient rpc_client0(grpc::CreateChannel(serverAddr0, grpc::InsecureChannelCredentials()), client_id, query_index, serverAddr0);
    DpfPirClient rpc_client1(grpc::CreateChannel(serverAddr1, grpc::InsecureChannelCredentials()), client_id, query_index, serverAddr1);

    /* GenFuncKeys */
    size_t logN = 22;
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> keys = DPF::Gen(query_index, logN);
    std::cout << "[" << client_id << "] "
              << "1.GenFuncKeys." << std::endl;

    /* PIR */
    hashdatastore::hash_type answer0 = rpc_client0.DpfPir(keys.first);
    hashdatastore::hash_type answer1 = rpc_client1.DpfPir(keys.second);

    /* Answer reconstructed */
    std::cout << "[" << client_id << "] "
              << "3.Answer reconstructed: " << std::endl;
    hashdatastore::hash_type answer = _mm256_xor_si256(answer0, answer1);
    std::cout << "\t"
              << "answer0:" << _mm256_extract_epi64(answer0, 0) << std::endl;
    std::cout << "\t"
              << "answer1:" << _mm256_extract_epi64(answer1, 0) << std::endl;
    std::cout << "\t"
              << "answer:" << _mm256_extract_epi64(answer, 0) << std::endl;

    return 0;
}