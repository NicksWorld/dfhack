-- A completion system for commands

-- Completion object structure:
-- CompletionDefinition{
--     subcommands = {["name"] = CompletionDefinition{}},
--     switches = {["--switch"] = SwitchDefinition{}}
-- }

-- Switch object structure:
-- SwitchDefinition{
--     argument_required = false,
--     arguments = {"possible", "argument", "completions", "here"}
-- }

local _ENV = mkmodule('completion')

local function contains(table, val)
    for _,v in pairs(table) do
        if v == val then
            return true
        end
    end
    return false
end

-- CompletionDefinition def
-- vector<string> args
-- bool editing_arg (if last arg is currently being edited)
function getCompletions(def, args, editing_last)
    if def == nil then
        error("Expected CompletionDefinition got nil")
    end
    if args == nil then
        args = {}
    end

    local possibleCompletions = {}

    -- Look for a matching subcommand first
    if (def.subcommands ~= nil and args[1] ~= nil) then
        for k,v in pairs(def.subcommands) do
            if k == args[1] then
                return getCompletions(v, table.unpack(args, 2), editing_last)
            end
        end
    end

    -- If first argument is not fully supplied, subcommands are possible
    if #args <= 1 then
        local shouldStartWith = editing_last and args[1] or ''

        for k,_ in pairs(def.subcommands or {}) do
            if k:find(shouldStartWith) == 1 then
                table.insert(possibleCompletions, k)
            end
        end
    end

    -- Check if currently editing a switch argument
    if #args >= 1 then
        local shouldStartWith = editing_last and args[#args] or ''
        
        for k,v in pairs(def.switches or {}) do
            if k == args[#args-1] then
                -- We are editing this switch
                if v.argument_required then
                    argWasRequired = true
                    possibleCompletions = {}
                end

                for _,arg in pairs(v.arguments or {}) do
                    if arg:find(shouldStartWith) == 1 then
                        table.insert(possibleCompletions, arg)
                    end
                end

                -- If argument is required, it can't be anything else
                if v.argument_required then
                    return possibleCompletions
                end
                break;
            end
        end
    end

    -- Check for any switch completions
    local shouldStartWith = editing_last and args[#args] or ''
    for k,_ in pairs(def.switches or {}) do
        if k:find(shouldStartWith) == 1 then
            table.insert(possibleCompletions, k)
        end
    end

    return possibleCompletions

end

return _ENV