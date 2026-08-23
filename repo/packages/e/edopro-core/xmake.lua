package("edopro-core")

    set_homepage("https://github.com/edo9300/ygopro-core")

    -- Build from the project's vendored core (pinned commit: see VENDORED.md)
    -- for rules parity with EDOPro 41.0.2, instead of cloning upstream HEAD.
    set_sourcedir(path.join(os.scriptdir(), "..", "..", "..", "..", "..", "..",
                            "simulator", "ygopro-core"))

    on_load(function (package)
        package:add("links", "edopro-core")
        -- The core includes <lua.h> without extern "C" on purpose: it must
        -- link a Lua built as C++ (exceptions instead of longjmp). Arch ships
        -- this as liblua++ (5.5).
        package:add("syslinks", "lua++5.5")
    end)

    on_install("linux", function (package)
        io.writefile("xmake.lua", [[
            add_rules("mode.debug", "mode.release")
            target("edopro-core")
                set_kind("static")
                set_languages("c++17")
                add_files("*.cpp")
                add_headerfiles("*.h")
                add_headerfiles("RNG/*.hpp")
        ]])
        import("package.tools.xmake").install(package)
        os.cp("*.h", package:installdir("include", "edopro-core"))
        os.cp("RNG", package:installdir("include", "edopro-core"))
    end)
package_end()
