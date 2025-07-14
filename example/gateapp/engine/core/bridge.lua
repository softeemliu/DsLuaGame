local network = require("engine.core.network")

function onNetworkAccept(sock, ip)
    network:onClientAccept(sock, ip)
end

function onNetworkMsg(sock, buf)
    network:onNetworkMsg(sock, buf)
end

function onNetworkWrite(sock, msgid, buff)
    network:onNetworkWrite(sock, msgid, buff)
end

function onNetworkError(sock, err)
    network:onNetworkError(sock, err)
end

function onScriptsRload(ver, target)
    Reload:onScriptsRload()
end;