package("edopro-core")

    set_homepage("https://github.com/edo9300/ygopro-core")

    -- Build from the parent project's vendored core (pinned submodule commit)
    -- for rules parity with EDOPro 41.0.2, with local patches applied to an
    -- out-of-tree copy so the submodule stays pristine (and free of build
    -- artifacts). Patches live in the parent project's bot/patches/.
    set_sourcedir(path.join(os.scriptdir(), "..", "..", "..", "..", "..", "..",
                            "simulator", "ygopro-core"))

    on_load(function (package)
        package:add("links", "edopro-core")
        -- The core includes <lua.h> without extern "C" on purpose: it must
        -- link a Lua built as C++ (exceptions instead of longjmp). EDOPro's
        -- core vendors Lua 5.4.8 (lua/src @ 6e22fed = tag v5.4.8), so we link
        -- the system C++ build of exactly that line, lua++5.4, and put its
        -- headers ahead of the default include path (where /usr/include/lua.h
        -- is 5.5). Lua 5.5 made for-loop variables read-only, which turns
        -- legal 5.4 card scripts into load errors (found 2026-09-06 via
        -- bot/tools/effect_dump.py: Ghostrick Socuteboss lost all effects).
        package:add("syslinks", "lua++5.4")
        -- NOTE: package:add("includedirs", ...) is rewritten relative to the
        -- install dir and suppresses the default include/ dir, so the header
        -- path goes through cxflags for consumers (the core's own compile
        -- adds it in the generated inner xmake.lua below).
        package:add("cxflags", "-I/usr/include/lua5.4")
    end)

    on_install("linux", function (package)
        local root = path.join(os.scriptdir(), "..", "..", "..", "..", "..", "..")
        local builddir = path.join(package:cachedir(), "patched-src")
        os.tryrm(builddir)
        os.mkdir(builddir)
        os.cp("*.cpp", builddir)
        os.cp("*.h", builddir)
        os.cp("RNG", builddir)
        local patches = {
            path.join(root, "bot", "patches", "ygopro-core-lua-budget.patch"),
        }
        os.cd(builddir)
        for _, p in ipairs(patches) do
            os.vrunv("patch", {"-p1", "-i", p})
        end
        io.writefile("xmake.lua", [[
            add_rules("mode.debug", "mode.release")
            target("edopro-core")
                set_kind("static")
                set_languages("c++17")
                add_includedirs("/usr/include/lua5.4")  -- EDOPro's Lua line (5.4.8), not system 5.5
                add_files("*.cpp")
                add_headerfiles("*.h")
                add_headerfiles("RNG/*.hpp")
        ]])
        import("package.tools.xmake").install(package)
        os.cp("*.h", package:installdir("include", "edopro-core"))
        os.cp("RNG", package:installdir("include", "edopro-core"))
    end)
package_end()
