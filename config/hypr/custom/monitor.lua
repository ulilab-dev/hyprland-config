------------------
---- MONITORS ----
------------------

-- See https://wiki.hypr.land/Configuring/Basics/Monitors/
hl.monitor({
    output   = "HDMI-A-1",
    mode     = "1920x1080@100",
    position = "0x0",
    scale    = "1",
})


--for w = 1, 9 do
--    hl.workspace_rule({
--        workspace = tostring(w),
--        monitor = "HDMI-A-1",
--        persistent = true
--    })
--end
