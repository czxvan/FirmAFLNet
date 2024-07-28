#include "crow_all.h"
int main()
{
    crow::SimpleApp app; //define your crow application

    std::atomic<int> request_count{0}; //define an atomic integer to keep track of the number of requests
    int data[15] = {0};
        
    //define your endpoint at the root directory
    CROW_ROUTE(app, "/")([&request_count](){
        ++request_count; //increment the request count
        return "Hello world\n";
    });

    CROW_ROUTE(app, "/count")([&request_count](){
        return "Request count: " + std::to_string(request_count) + "\n";
    });

    CROW_ROUTE(app, "/data/<int>")([&request_count, &data](int n){
        ++request_count;
        return "data: " + std::to_string(n) + " = " + std::to_string(data[n]) + "\n";
    });

    CROW_ROUTE(app, "/null")([](){
        int* ptr = NULL;
        return "data: " + std::to_string(*ptr) + "\n";
    });


    //set the port, set the app to run on multiple threads, and run the app
    app.port(8084)
    .ssl_file("/pem/cert.pem", "/pem/key.pem")
    .run();
}