"""Export real current source excerpts for report Figures 3-5; no duplicate implementation."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
out=Path(__file__).resolve().parent/'code'
out.mkdir(exist_ok=True)
items=[('Figure3_impact_generation','src/core/gameplay/ParticleEffects.cpp','void ParticleEffects::emit','void ParticleEffects::update'),
 ('Figure4_ground_fire','src/core/gameplay/ParticleEffects.cpp','  // Preserve the pre-effects fire','  lights_.clear();'),
 ('Figure4_original_trail','src/graphics/ArenaRenderer.cpp','void ArenaRenderer::drawFirebombTrail','}  // namespace craft::graphics'),
 ('Figure5_committed_route','src/core/gameplay/Gameplay.cpp','    const auto previousPosition = e.position;','    if (!e.waypoint) {'),
 ('Figure5_wait_and_recovery','src/core/gameplay/Gameplay.cpp','    const auto needsWallReconnect','    const auto combined = profile.halfExtents + playerHalfExtents();')]
for name,file,start,end in items:
 lines=(root/file).read_text().splitlines()
 a=next(i for i,s in enumerate(lines) if start in s)
 b=next(i for i in range(a+1,len(lines)) if end in lines[i])
 text='SOURCE: '+file+'\nOpen the original file in Visual Studio for a syntax-highlighted screenshot.\n\n'
 text+='\n'.join(f'{i+1:4}: {lines[i]}' for i in range(a,b))+'\n'
 (out/(name+'.txt')).write_text(text)
 print(name,file,a+1,b)
