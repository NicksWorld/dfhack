#include "modules/Completion.h"
#include "ColorText.h"
#include "Core.h"
#include "modules/Filesystem.h"
#include "json/json.h"
#include <optional>

/*void
DFHack::Completion::getCompletions(CompletionDefinition const& completion,
                                   vector<string> const& args,
                                   size_t editing_arg,
                                   vector<CompletionResult> &results) {
    results.clear();
    
    if (args.size() >= 1) {
        // Look for any matching subcommands
        for (auto const& subcommand : completion.subcommands) {
            if (args[0] == subcommand.first) {
                // Subcommand matched, parse args from subcommand
                vector<string> remainingArgs(args.begin() + 1, args.end());
                DFHack::Completion::getCompletions(subcommand.second, remainingArgs, editing_arg - 1, results);
            }
        }
    }

    // If first argument is not fully supplied, subcommands are possible
    if (args.size() <= 1 && editing_arg == 0) {
        string shouldStartWith = args.size() == 0 ? "" : args[0];

        for (auto const& subcommand : completion.subcommands) {
            if (subcommand.first.find(shouldStartWith) == 0) {
                CompletionResult res;
                res.mode = CompletionMode::ReplaceCurrent;
                res.completion = subcommand.first;
                results.push_back(res);
            }
        }
    }

    // Check if currently editing a switch argument
    if (args.size() >= 1 && editing_arg >= 1) {
        bool isArgRequired = false;
        std::vector<CompletionResult> argCompletions;

        string shouldStartWith = args.size() > editing_arg ? args[editing_arg] : "";
        for (auto const& sw : completion.switches) {
            if (sw.first == args[editing_arg-1]) {
                isArgRequired = sw.second.argument_required;
                for (auto const& argument : sw.second.argument_completions) {
                    if (argument.find(shouldStartWith) == 0) {
                        CompletionResult res;
                        res.mode = CompletionMode::ReplaceCurrent;
                        res.completion = argument;
                        argCompletions.push_back(res);
                    }
                }
                break;
            }
        }

        if (isArgRequired) {
            results.clear();
            results.reserve(argCompletions.size());
            results.insert(results.end(), argCompletions.begin(), argCompletions.end());
            return;
        } else {
            results.reserve(results.size() + argCompletions.size());
            results.insert(results.end(), argCompletions.begin(), argCompletions.end());
        }
    }

    // Check for any switch completions
    string shouldStartWith = args.size() - 1 == editing_arg ? args[editing_arg] : "";
    for (auto const& sw : completion.switches) {
        if (sw.first.find(shouldStartWith) == 0) {
            CompletionResult res;
            res.mode = CompletionMode::ReplaceCurrent;
            res.completion = sw.first;
            results.push_back(res);
        }
    }
    return;
}*/

DFHack::Completion::CompletionDefinition* DFHack::Completion::loadCompletionDefinition(color_ostream& con, std::string command_name) {
    std::string filename("hack/data/completions/" + command_name + ".json");

    if (!DFHack::Filesystem::isfile(filename)) {
        con.printerr("Couldn't find completion json\n");
        return nullptr;
    }

    Json::Value completionJson;

    std::ifstream inFile(filename, std::ios_base::in);
    try {
        if (inFile.is_open()) {
            inFile >> completionJson;
        }
    } catch (const std::exception &e) {
        con.printerr("Failed to read completion file\n");
        return nullptr;
    }
    inFile.close();

    // Check if completion is actually in a lua module
    try {
        std::string luaModule = completionJson.get("lua", "").asString();
    } catch (const std::exception &e) {
        con.printerr("Failed to read lua\n");
        return nullptr;
    }

    return nullptr;
}

DFHACK_EXPORT int DFHack::Completion::test() {
    return 1;
}