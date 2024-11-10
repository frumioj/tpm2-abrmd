-module(tpm2_abrmd).
-export([start_session/0, end_session/1]).

start_session() ->
    call_port({start_session}).

end_session(Handle) ->
    call_port({end_session, Handle}).

call_port(Msg) ->
    % Implementation of port communication
    % Send message to C port and receive response 