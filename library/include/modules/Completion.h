#pragma once
#include "Export.h"
#include "ColorText.h"
#include "Internal.h"

#include <map>
#include <string>
#include <vector>
#include <optional>

namespace DFHack {
    namespace Completion {

        enum class CompletionMode { ReplaceCurrent, AddNew };

        struct CompletionResult {
            CompletionMode mode;
            std::string completion;
        };

        struct SwitchDefinition {
            bool argument_required = false;
            std::vector<std::string> argument_completions;
        };

        struct CompletionDefinition {
            // Available subcommands
            std::map<std::string, CompletionDefinition> subcommands;
            std::map<std::string, SwitchDefinition> switches;
        };

        //DFHACK_EXPORT void getCompletions(CompletionDefinition const& completion, std::vector<std::string> const& args, size_t editing_arg, std::vector<CompletionResult> &results);

        DFHACK_EXPORT CompletionDefinition* loadCompletionDefinition(color_ostream& con, std::string command_name);

        DFHACK_EXPORT int test();
    }
}