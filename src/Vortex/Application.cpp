#include <Precomp.h>

#include <Core/Common/Input.h>
#include <Core/Common/Event.h>
#include <Vortex/Shortcuts.h>

#include <Core/Utils/Unicode.h>
#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Xmr.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>
#include <Core/System/Debug.h>
#include <Core/System/Log.h>
#include <Core/System/AudioOut.h>

#include <Core/Graphics/Shader.h>
#include <Core/Graphics/Renderer.h>
#include <Core/Graphics/FontManager.h>
#include <Core/Graphics/Vulkan.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/UiMan.h>

#include <Simfile/Simfile.h>
#include <Simfile/NoteSet.h>
#include <Simfile/History.h>

#include <Vortex/Editor.h>
#include <Vortex/Application.h>

#include <Vortex/View/View.h>
#include <Vortex/View/Minimap.h>
#include <Vortex/View/TextOverlay.h>
#include <Vortex/View/Hud.h>
#include <Vortex/View/StatusBar.h>
#include <Vortex/View/CommandLine.h>
#include <Vortex/Notefield/Notefield.h>
#include <Vortex/Notefield/SegmentBoxes.h>
#include <Vortex/Notefield/Waveform.h>

#include <Vortex/Edit/Editing.h>
#include <Vortex/Edit/Selection.h>
#include <Vortex/Edit/StreamGenerator.h>
#include <Vortex/Edit/TempoTweaker.h>

#include <Vortex/Managers/MusicMan.h>
#include <Vortex/Managers/GameMan.h>
#include <Vortex/Noteskin/NoteskinMan.h>
#include <Vortex/Managers/DialogMan.h>
#include <Vortex/ApplicationMenu.h>
#include <Vortex/Lua/Global.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Static data.

typedef void (*Initializer)();
typedef void (*InitializerWithSettings)(const XmrDoc&);
typedef void (*Deinitializer)();

namespace Application
{
	struct State
	{
		bool keepRunning = true;
	
		Sprite logo;
		bool logoLoaded = false;
	
		vector<Deinitializer> deinitializers;
	
		EventSubscriptions subscriptions;
	};
	static State* state = nullptr;
}
using Application::state;

static void Register(Initializer initialize, Deinitializer deinitialize)
{
	initialize();
	state->deinitializers.push_back(deinitialize);
}

static void Register(InitializerWithSettings initialize, Deinitializer deinitialize, const XmrDoc& settings)
{
	initialize(settings);
	state->deinitializers.push_back(deinitialize);
}

// =====================================================================================================================
// Logo graphic.

static const char* LogoPath = "assets/arrow vortex logo.png";

static void DrawLogo()
{
	auto sim = Editor::currentSimfile();
	if (sim)
	{
		if (state->logoLoaded)
		{
			state->logo.texture.reset();
			state->logoLoaded = false;
		}
	}
	else
	{
		if (!state->logoLoaded)
		{
			auto path = FilePath(LogoPath);
			state->logo.texture = make_shared<Texture>(path, TextureFormat::Alpha);
			state->logoLoaded = true;
		}

		Size2 size = Window::getSize();
		Rect view = Rect(0, 0, size.w, size.h);
		Draw::fill(view, Color(38, 38, 38, 255));
		state->logo.draw(view.center(), Color(255, 255, 255, 26));
	}
}

// =====================================================================================================================
// Command line arguments.

struct CommandLineScriptArgs
{
	string name;
	vector<string> unnamedArgs;
	map<string, string> namedArgs;
};

struct CommandLineArgs
{
	string simfilePath;
	bool openConsole = false;
	unique_ptr<CommandLineScriptArgs> script;
};

static CommandLineArgs ParseCommandLineArgs()
{
	CommandLineArgs result;
	result.openConsole = true;
	auto args = System::getCommandLineArgs();
	for (size_t i = 1; i < args.size(); ++i)
	{
		if (args[i].starts_with('-'))
		{
			if (args[i] == "--console")
			{
				result.openConsole = true;
			}
			else if (args[i] == "--simfile")
			{
				if (++i < args.size())
					result.simfilePath = args[i];
				else
					Log::error("Command line option --simfile was not followed by a path.");
			}
			else if (args[i] == "--script")
			{
				result.script = make_unique<CommandLineScriptArgs>();
				if (++i < args.size())
					result.script->name = args[i];
				else
					Log::error("Command line option --script was not followed by a name.");

				while (++i < args.size())
				{
					if (args[i].starts_with("--"))
					{
						auto key = args[i].substr(2);
						if (++i < args.size())
							result.script->namedArgs[key] = args[i];
						else
							Log::error(format("Command line option --{} was not followed by a string.", key));
					}
					else
					{
						result.script->unnamedArgs.push_back(args[i]);
					}
				}
			}
			else
			{
				Log::error(format("Unknown command line option '{}'.", args[i]));
			}
		}
		else if (i == 1 && !args[i].starts_with("--"))
		{
			result.simfilePath = args[i];
		}
		else
		{
			Log::error(format("Invalid command line option '{}'.", args[i]));
		}
	}
	return result;
}

// =====================================================================================================================
// Core components.

static void InitializeCore(CommandLineArgs& args)
{
	#define REG(x) Register(x::initialize, x::deinitialize)

	// Register the system singletons first.
	REG(AudioOut);

	// Create the main window to get an OpenGL context.
	REG(Window);
	Window::setTitle("ArrowVortex");

	// Register the core singletons after the OpenGL context is available.
	REG(Renderer);
	REG(FontManager);
	REG(Text);
	REG(UiMan);

	#undef REG
}

// =====================================================================================================================
// Event handling.

static void HandleFileDrop(const System::FileDropped& evt)
{
	if (evt.paths.size() >= 1)
		Editor::openSimfile(evt.paths[0]);
}

static void HandleCloseRequested()
{
	if (Editor::closeSimfile())
		Application::exit();
}

// =====================================================================================================================
// Arrow Vortex components.

static void InitializeArrowVortex(CommandLineArgs& args)
{
	XmrDoc settings;
	settings.loadFile("settings/settings.txt");

	#define REG(x) Register(x::initialize, x::deinitialize, settings)

	REG(EventSystem);
	REG(Settings);
	REG(GameModeMan);
	REG(GameMan);
	REG(NoteskinMan);
	REG(TempoTweaker);

	REG(Editor);
	REG(MusicMan);
	REG(Editing);

	REG(Notefield);
	REG(Minimap);
	REG(StatusBar);
	REG(View);
	REG(Hud);
	REG(DialogMan);
	REG(TextOverlay);
	REG(CommandLine);

	REG(Selection);
	REG(Waveform);

	REG(Lua);
	REG(Commands);
	REG(Shortcuts);

	REG(ApplicationMenu);

	#undef REG

	// Enable or disable vsync.
	// if (Glx::supportsSwapInterval)
	// {
	// 	if (Application::useVerticalSync.value())
	// 	{
	// 		Glx::glSwapInterval(1);
	// 		Log::info("Vertical Sync :: Enabled");
	// 	}
	// 	else
	// 	{
	// 		Glx::glSwapInterval(0);
	// 		Log::info("Vertical Sync :: Disabled");
	// 	}
	// }
	Log::lineBreak();

	// Initialize Vulkan demo.
	Vulkan::initializeDemo();

	// Set the default text style.
	auto& regular = UiText::regular;
	auto& generalSettings = AV::Settings::general();
	regular.font = Font(FilePath(generalSettings.fontPath), FontHint::Auto);
	regular.fontSize = generalSettings.fontSize;

	// Register events.
	state->subscriptions.add<System::FileDropped>(HandleFileDrop);
	state->subscriptions.add<System::CloseRequested>(HandleCloseRequested);

	// Open simfile if provided.
	if (!args.simfilePath.empty())
		Editor::openSimfile(args.simfilePath);

	// Show the window.
	Window::show();
}

// =====================================================================================================================
// Application loop.

static void UpdateTitle()
{
	string title, subtitle;
	bool unsavedChanges = false;
	auto sim = Editor::currentSimfile();
	if (sim)
	{
		title = sim->title.get();
		subtitle = sim->subtitle.get();
		unsavedChanges = sim->hasUnsavedChanges();
	}
	if (title.length() || subtitle.length())
	{
		if (title.length() && subtitle.length()) title += " ";
		title += subtitle;
		if (unsavedChanges) title += "*";
		title += " :: ArrowVortex";
	}
	else
	{
		title = "ArrowVortex";
		if (unsavedChanges) title += "*";
	}
	Window::setTitle(title);
}

static void ApplicationLoop()
{
	// Process all windows messages.
	Window::handleMessages();

	// The cursor of the previous frame is set during message handling,
	// so the cursor for the upcoming frame is reset after the message handling.
	Window::setCursor(Window::CursorIcon::Arrow);

	// Free cached glyphs that have not been recently used.
	FontManager::optimizeGlyphPages();

	// Tick application.
	UiMan::tick();
	UpdateTitle();

	// Draw application.
	if (Vulkan::startFrame())
	{
		DrawLogo();
		UiMan::draw();
		Vulkan::drawDemo();
		Vulkan::finishFrame();
	}
}

// =====================================================================================================================
// API.

void Application::exit()
{
	state->keepRunning = false;
}

// =====================================================================================================================
// Program main.

static void InitializeApplication()
{
	state = new Application::State();

	auto args = ParseCommandLineArgs();

	// Open a console window for the log output.
	if (args.openConsole)
		Log::openConsole();

	// Log the application start-up startTime.
	Log::openFile();
	Log::info(format("Starting ArrowVortex :: {}", System::getLocalTime()));
	Log::info(format("Build: {}", System::getBuildDate()));
	Log::lineBreak();

	InitializeCore(args);
	InitializeArrowVortex(args);

	// If a script was specified, run it.
	auto script = args.script.get();
	if (script)
	{
		Log::info(format("Running script: %s", script->name));

		// TODO: support running scripts from command line?
		// Editor::runScript(script->name, script->unnamedArgs, script->namedArgs);
	}
}

static void ShutdownApplication()
{
	// Log the application termination startTime.
	Log::lineBreak();
	Log::info(format("Closing ArrowVortex :: {}", System::getLocalTime()));

	// Save the application settings.
	XmrDoc settings;
	Settings::save(settings);
	settings.saveFile("settings/settings.txt", XmrSaveSettings());

	// Release texture before graphics are deinitialized.
	state->logo.texture.reset();

	// Release subscriptions before event system is deinitialized.
	state->subscriptions.clear();

	// Destroy singletons in reverse order.
	for (auto deinitialize : reverseIterate(state->deinitializers))
		deinitialize();

	Util::reset(state);
}

int ApplicationMain()
{
	InitializeApplication();
	while (state->keepRunning)
		ApplicationLoop();
	ShutdownApplication();
	return 0;
}

} // namespace AV
