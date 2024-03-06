

workspace "Netweave"
	architecture "x64"
	startproject "Simple client"

	configurations {
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "NetCommon"
	location "NetCommon"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files {
		"%{prj.name}/**.h",
		"%{prj.name}/**.hpp",
		"%{prj.name}/**.inl",
		"%{prj.name}/**.cpp",
		"%{prj.name}/**.c",
	}

	includedirs {
		"Libraries/include",
	}

	libdirs {
		"Libraries/lib",
	}

	links {
	}

	defines {
		"NETCOMMON_BUILD_DLL",
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "NETCOMMON_DEBUG"
		symbols "on"
		runtime "Debug"

	filter "configurations:Release"
		defines "NETCOMMON_RELEASE"
		optimize "on"
		runtime "Release"

	filter "configurations:Dist"
		defines "NETCOMMON_DIST"
		optimize "on"
		runtime "Release"

project "Simple client"
	location "Simple client"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files {
		"%{prj.name}/**.h",
		"%{prj.name}/**.hpp",
		"%{prj.name}/**.inl",
		"%{prj.name}/**.cpp",
	}

	includedirs {
		"Libraries/include",
		"NetCommon",
	}

	libdirs {
		"Libraries/lib",
	}

	links {
		"NetCommon",
	}

	filter "system:windows"
		systemversion "latest"

project "Simple server"
	location "Simple server"
	kind "ConsoleApp"
	language "C++"
	staticruntime "on"
	cppdialect "C++17"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files {
		"%{prj.name}/**.h",
		"%{prj.name}/**.c",
		"%{prj.name}/**.hpp",
		"%{prj.name}/**.inl",
		"%{prj.name}/**.cpp",
	}

	includedirs {
		"Libraries/include",
		"NetCommon",
	}

	libdirs {
		"Libraries/lib",
	}

	links {
		"NetCommon",
	}

	filter "system:windows"
		systemversion "latest"
