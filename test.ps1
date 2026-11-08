$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force build | Out-Null
    $compiler = (Get-Command g++ -ErrorAction Stop).Source
    $flags = @('-std=c++20', '-O2', '-Wall', '-Wextra', '-DFREEGLUT_STATIC')
    $libs = @('-static', '-lfreeglut', '-lopengl32', '-lglu32', '-lgdi32', '-lwinmm')
    $core = @('src/Player.cpp', 'src/Guard.cpp', 'src/Character.cpp', 'src/Collision.cpp',
              'src/Level.cpp', 'src/Map.cpp', 'src/Graphics.cpp', 'src/Navigation.cpp')
    $game = @('src/Game.cpp', 'src/UI.cpp', 'src/Scoreboard.cpp', 'src/Campus.cpp')
    foreach ($name in @('movement_test', 'vision_test', 'audio_events_test', 'gameplay_test')) {
        $sources = @("tests/$name.cpp") + $core
        if ($name -eq 'audio_events_test' -or $name -eq 'gameplay_test') { $sources += $game }
        if ($name -eq 'gameplay_test') { $sources += 'src/Audio.cpp' }
        & $compiler @flags @sources -o "build/$name.exe" @libs
        if ($LASTEXITCODE -ne 0) { throw "Build failed: $name" }
        & "./build/$name.exe"
        if ($LASTEXITCODE -ne 0) { throw "Tests failed: $name" }
    }
} finally {
    Pop-Location
}
