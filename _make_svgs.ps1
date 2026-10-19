$ErrorActionPreference = 'Stop'
$root = (Get-Location).Path

function Xml([string]$s) { [System.Security.SecurityElement]::Escape($s) }

# Face images are flat 2D planning views. The dotted background helps estimate
# placement; it is not a recommendation to make cube walls from perfboard.
$faces = @(
    [pscustomobject]@{
        key='front'; title='FRONT FACE'; subtitle='OLED display faces outward through a front-wall opening.'
        parts=@(
            '<g><rect x="355" y="232" width="190" height="118" rx="10" fill="#111827" stroke="#334155" stroke-width="4"/><rect x="371" y="248" width="158" height="84" rx="4" fill="#0f172a"/><text x="405" y="296" font-family="sans-serif" font-size="22" font-weight="700" fill="#a7f3d0">OLED</text><text x="407" y="344" font-family="sans-serif" font-size="10" fill="#334155">display module PCB</text></g>',
            '<g><rect x="380" y="360" width="140" height="30" rx="4" fill="#e2e8f0" stroke="#475569" stroke-width="2"/><g fill="#d6a84f" stroke="#92400e"><circle cx="395" cy="375" r="5"/><circle cx="425" cy="375" r="5"/><circle cx="455" cy="375" r="5"/><circle cx="485" cy="375" r="5"/></g><text x="390" y="414" font-family="sans-serif" font-size="11" fill="#334155">4-pin header: GND VCC SCL SDA*</text></g>'
        )
        route='<path d="M395 380 L330 420 L290 420" fill="none" stroke="#2563eb" stroke-width="5"/><path d="M425 380 L335 435 L290 435" fill="none" stroke="#2563eb" stroke-width="5"/><path d="M455 380 L345 450 L290 450" fill="none" stroke="#dc2626" stroke-width="5"/><path d="M485 380 L350 465 L290 465" fill="none" stroke="#111827" stroke-width="5"/><text x="545" y="432" font-family="sans-serif" font-size="12" fill="#1e3a8a">SDA / SCL</text><text x="545" y="456" font-family="sans-serif" font-size="12" fill="#334155">3V3 / GND</text>'
        focus='FRONT FACE FOCUS: OLED window and four labeled connections.'
        actions=@(
            @('Put the flat face-planning board on the table. Mark FRONT at its top edge.','No component yet. Keep the actual cube wall separate from this dotted layout.','Check: your plan leaves enough room for the measured display and its cable.'),
            @('Set the OLED module on the center marks with its screen facing you.','Trace the display opening from the real module; mark header pin labels.','Check: the module fits without covering the front-wall screw points.'),
            @('Add the four-pin header below the OLED, matching its printed labels.','The OLED module is fixed to the front wall; its header/wires reach inward.','Check: GND, VCC, SCL and SDA order matches your exact OLED board.'),
            @('Route SDA/SCL in blue, 3V3 in red, and GND in black toward the inside.','Leave a loose bend so the front wall can open without pulling wires.','Check: no wire crosses a sharp edge or reaches a wrong pin.'),
            @('Solder one labeled wire at a time to its matching header pin.','Heat the pin and wire together; touch solder to the joint, then let cool.','Check: gently tug each wire; inspect that neighboring pins are not bridged.')
        )
    },
    [pscustomobject]@{
        key='back'; title='BACK FACE'; subtitle='Keep separate access openings for the charger and ESP32 USB-C sockets.'
        parts=@(
            '<g><rect x="340" y="225" width="170" height="112" rx="8" fill="#3b82f6" stroke="#1d4ed8" stroke-width="3"/><rect x="327" y="263" width="28" height="38" rx="7" fill="#cbd5e1" stroke="#334155" stroke-width="3"/><rect x="374" y="243" width="32" height="28" rx="3" fill="#111827"/><rect x="420" y="243" width="32" height="28" rx="3" fill="#111827"/><circle cx="476" cy="252" r="5" fill="#ef4444"/><circle cx="490" cy="252" r="5" fill="#22c55e"/><text x="375" y="300" font-family="sans-serif" font-size="16" font-weight="700" fill="#fff">TP4056</text><text x="376" y="320" font-family="sans-serif" font-size="11" fill="#fff">charger board</text></g>',
            '<g><rect x="290" y="372" width="108" height="38" rx="12" fill="#334155" stroke="#0f172a" stroke-width="3"/><text x="405" y="397" font-family="sans-serif" font-size="12" fill="#334155">ESP32 USB-C opening</text><rect x="300" y="218" width="250" height="128" rx="7" fill="none" stroke="#f59e0b" stroke-width="3" stroke-dasharray="8 5"/><text x="517" y="277" font-family="sans-serif" font-size="11" fill="#78350f">charger USB-C</text></g>'
        )
        route='<path d="M342 333 L310 430 L275 430" fill="none" stroke="#dc2626" stroke-width="5"/><path d="M356 333 L320 445 L275 445" fill="none" stroke="#111827" stroke-width="5"/><text x="490" y="430" font-family="sans-serif" font-size="12" fill="#991b1b">BAT+ / GND</text>'
        focus='BACK FACE FOCUS: TP4056 USB-C access; ESP32 USB-C is a different port.'
        actions=@(
            @('Mark BACK on a flat face layout. Put the charger USB-C at an accessible edge.','Keep the ESP32 USB-C access mark separate from the charger opening.','Check: two different cables can reach without colliding.'),
            @('Place the TP4056 module behind the charger opening; do not solder its case.','Match the real USB-C socket position and the board pads B+, B-, OUT+, OUT-.','Check: opening is large enough for a cable plug; no wall presses the board.'),
            @('Add the separate ESP32 USB-C access opening as a clearance mark.','The DevKit mounts on the bottom/internal shelf, not this back-face board.','Check: the two port openings do not overlap in the real enclosure.'),
            @('Plan protected battery wires from OUT+/- toward the inside power switch.','Keep charger input (USB-C) distinct from battery pads and load output pads.','Check: you can read B+/B- and OUT+/OUT- labels before soldering.'),
            @('Solder only short insulated pigtails to clearly identified charger pads.','Do not solder the LiPo directly to its pouch; use its protected leads.','Check: verify polarity with the meter before connecting the cell.')
        )
    },
    [pscustomobject]@{
        key='top'; title='TOP FACE'; subtitle='Top carries touch, pushbutton, and slide switch controls.'
        parts=@(
            '<g><rect x="270" y="235" width="105" height="85" rx="9" fill="#15803d" stroke="#166534" stroke-width="3"/><circle cx="322" cy="276" r="31" fill="#d6a84f" stroke="#fef3c7" stroke-width="4"/><rect x="305" y="286" width="16" height="11" fill="#94a3b8"/><text x="270" y="340" font-family="sans-serif" font-size="12" fill="#334155">TTP223 touch board</text></g>',
            '<g><rect x="405" y="250" width="62" height="60" rx="9" fill="#1f2937" stroke="#111827" stroke-width="3"/><circle cx="436" cy="244" r="24" fill="#dc2626" stroke="#991b1b" stroke-width="3"/><path d="M417 309v20M455 309v20M417 329h38" stroke="#94a3b8" stroke-width="4"/><rect x="500" y="258" width="75" height="48" rx="7" fill="#94a3b8" stroke="#475569" stroke-width="3"/><rect x="520" y="267" width="28" height="30" rx="5" fill="#475569"/><path d="M512 306v20M532 306v20M562 306v20" stroke="#d6a84f" stroke-width="4"/><text x="398" y="350" font-family="sans-serif" font-size="11" fill="#334155">tactile button</text><text x="502" y="350" font-family="sans-serif" font-size="11" fill="#334155">SPDT switch</text></g>'
        )
        route='<path d="M286 315L270 420L235 420" fill="none" stroke="#16a34a" stroke-width="5"/><path d="M430 325L310 445L235 445" fill="none" stroke="#16a34a" stroke-width="5"/><path d="M520 323L325 465L235 465" fill="none" stroke="#7c3aed" stroke-width="5"/><text x="485" y="423" font-family="sans-serif" font-size="12" fill="#166534">TOUCH GPIO32</text><text x="485" y="450" font-family="sans-serif" font-size="12" fill="#166534">BUTTON GPIO25</text><text x="485" y="476" font-family="sans-serif" font-size="12" fill="#5b21b6">QUIET GPIO26</text>'
        focus='TOP FACE FOCUS: keep the touch pad, red key, and slider distinct.'
        actions=@(
            @('Mark FRONT direction on the dotted layout so controls face the user.','Choose positions with room for fingers and for each module behind the wall.','Check: none of the three controls touches another part.'),
            @('Place the TTP223 board under the planned touch area.','Keep its gold touch pad under thin plastic, wood, or another non-metal cover.','Check: metal screws and wires are not directly over its touch pad.'),
            @('Place the red tactile button and SPDT slide switch at separate marks.','Mark the button body opening and the narrow switch-slider slot from real parts.','Check: button moves freely and switch reaches both positions.'),
            @('Route TTP223 OUT and button signal wires to separate ESP32 pins.','SPDT is a quiet-mode GPIO switch, not the battery power switch.','Check: all module pins have labels; no bare wires touch.'),
            @('Solder labeled control wires with power disconnected.','Connect SPDT common to GPIO26 and one throw to GND for quiet mode.','Check: unused switch throw stays open; never connect battery voltage to it.')
        )
    },
    [pscustomobject]@{
        key='bottom'; title='BOTTOM / INTERNAL SHELF'; subtitle='This is the one main perfboard shelf inside the cube, held above the bottom wall.'
        parts=@(
            '<g><rect x="290" y="186" width="178" height="232" rx="12" fill="#14532d" stroke="#064e3b" stroke-width="4"/><rect x="322" y="232" width="114" height="105" rx="9" fill="#94a3b8" stroke="#e2e8f0" stroke-width="4"/><text x="336" y="276" font-family="sans-serif" font-size="14" fill="#fff">ESP-WROOM-32</text><text x="348" y="297" font-family="sans-serif" font-size="12" fill="#fff">shield</text><path d="M306 199h146v24H306z" fill="#111827"/><text x="338" y="216" font-family="sans-serif" font-size="10" fill="#fff">ANTENNA</text><path d="M347 418v20h64v-20" fill="#cbd5e1" stroke="#334155" stroke-width="3"/><text x="345" y="458" font-family="sans-serif" font-size="12" fill="#334155">USB-C end</text></g>',
            '<g><rect x="490" y="208" width="110" height="72" rx="8" fill="#3b82f6" stroke="#1d4ed8" stroke-width="3"/><rect x="480" y="230" width="20" height="30" rx="6" fill="#cbd5e1" stroke="#334155" stroke-width="2"/><text x="512" y="250" font-family="sans-serif" font-size="13" font-weight="700" fill="#fff">TP4056</text><rect x="490" y="307" width="110" height="55" rx="20" fill="#fef3c7" stroke="#b45309" stroke-width="3"/><path d="M493 350v18" stroke="#dc2626" stroke-width="4"/><path d="M503 350v18" stroke="#111827" stroke-width="4"/><text x="516" y="340" font-family="sans-serif" font-size="12" fill="#334155">LiPo 500 mAh</text><rect x="490" y="380" width="110" height="45" rx="6" fill="#16a34a" stroke="#14532d" stroke-width="3"/><rect x="532" y="390" width="22" height="18" rx="3" fill="#1f2937"/><text x="496" y="441" font-family="sans-serif" font-size="11" fill="#166534">extra 3.3 V regulator</text><ellipse cx="250" cy="375" rx="25" ry="17" fill="#111827"/><ellipse cx="250" cy="368" rx="25" ry="9" fill="#1f2937"/><circle cx="250" cy="368" r="5" fill="#020617"/><text x="220" y="405" font-family="sans-serif" font-size="11" fill="#334155">active buzzer</text></g>'
        )
        route='<path d="M490 266L468 300L445 300" fill="none" stroke="#dc2626" stroke-width="5"/><path d="M490 276L475 315L445 315" fill="none" stroke="#111827" stroke-width="5"/><path d="M275 375L300 350L330 350" fill="none" stroke="#16a34a" stroke-width="5"/><text x="467" y="390" font-family="sans-serif" font-size="12" fill="#78350f">Battery is held beside board;</text><text x="467" y="410" font-family="sans-serif" font-size="12" fill="#78350f">never solder to the pouch.</text>'
        focus='INTERNAL SHELF FOCUS: ESP32, charger, battery, buzzer and regulator space.'
        actions=@(
            @('Use this dotted board as the internal shelf, not as a cube wall.','Leave room for socket rows, wire loops, charger, regulator, and buzzer.','Check: the shelf and all parts fit inside your measured cube mock-up.'),
            @('Fit two female socket rows so the ESP32 can be removed.','Point its USB-C end toward the separate back-wall access opening.','Check: antenna end has nearby plastic, not battery or metal.'),
            @('Add the charger and regulator only if they fit without crowding.','Place the active buzzer with its sound hole open; put the LiPo beside the board.','Check: the LiPo pouch cannot touch solder legs or screw tips.'),
            @('Plan red regulated supply and black GND wires between power parts.','Keep charger battery pads separate from the regulated ESP32 supply.','Check: raw LiPo voltage does not go directly to ESP32 3V3 or 5V.'),
            @('Solder socket pins and module-wire landing points one at a time.','Use standoffs to hold this flat board above the bottom cube wall.','Check: no bare solder leg can press into or puncture the battery pouch.')
        )
    },
    [pscustomobject]@{
        key='left'; title='LEFT FACE'; subtitle='The round INMP441 microphone board sits behind a clear sound opening.'
        parts=@(
            '<g><circle cx="450" cy="278" r="80" fill="#111827" stroke="#475569" stroke-width="5"/><circle cx="450" cy="278" r="37" fill="#020617"/><circle cx="450" cy="278" r="10" fill="#475569"/><circle cx="405" cy="228" r="8" fill="#d6a84f"/><circle cx="495" cy="228" r="8" fill="#d6a84f"/><circle cx="405" cy="328" r="8" fill="#d6a84f"/><circle cx="495" cy="328" r="8" fill="#d6a84f"/><text x="380" y="380" font-family="sans-serif" font-size="14" fill="#334155">INMP441 round breakout</text></g>',
            '<g><rect x="365" y="397" width="170" height="35" rx="4" fill="#e2e8f0" stroke="#475569" stroke-width="2"/><g fill="#d6a84f"><circle cx="378" cy="414" r="5"/><circle cx="408" cy="414" r="5"/><circle cx="438" cy="414" r="5"/><circle cx="468" cy="414" r="5"/><circle cx="498" cy="414" r="5"/><circle cx="528" cy="414" r="5"/></g><text x="373" y="454" font-family="sans-serif" font-size="11" fill="#334155">VDD GND L/R WS SCK SD*</text></g>'
        )
        route='<path d="M378 420L320 430L280 430M408 420L320 445L280 445" fill="none" stroke="#dc2626" stroke-width="4"/><path d="M438 420L315 458L280 458M468 420L310 470L280 470M498 420L305 482L280 482" fill="none" stroke="#eab308" stroke-width="5"/><path d="M528 420L300 495L280 495" fill="none" stroke="#111827" stroke-width="4"/><text x="545" y="468" font-family="sans-serif" font-size="12" fill="#854d0e">SCK 18 / WS 19 / SD 35</text>'
        focus='LEFT FACE FOCUS: round microphone board and open sound port.'
        actions=@(
            @('Mark LEFT on the flat dotted face layout.','Set the round mic where its center sound opening can face outward.','Check: the opening is clear and away from the buzzer side.'),
            @('Place the actual round INMP441 board over its intended opening.','Trace its edge and mounting/pin locations; do not drill until measured.','Check: no wall material covers the mic sound port.'),
            @('Add or use the six-pin header with the printed pin names.','Keep wire exits toward the cube interior, not outside the case.','Check: identify VDD, GND, L/R, WS, SCK and SD on your board.'),
            @('Route VDD red, GND black, and SCK/WS/SD yellow to the shelf.','Use GPIO18 for SCK, GPIO19 for WS, and GPIO35 for SD to match firmware.','Check: mic receives 3.3 V only and each wire reaches its matching pin.'),
            @('Solder each labeled mic lead to its matching header pad.','Heat pad and wire together; add only enough solder to wet both.','Check: no solder joins neighboring microphone pins.')
        )
    },
    [pscustomobject]@{
        key='right'; title='RIGHT FACE'; subtitle='The BME280 needs open vents so room air can reach its sensor.'
        parts=@(
            '<g><rect x="365" y="215" width="170" height="190" rx="10" fill="#15803d" stroke="#166534" stroke-width="4"/><circle cx="510" cy="235" r="11" fill="#f8fafc" stroke="#334155" stroke-width="2"/><rect x="405" y="265" width="82" height="64" rx="7" fill="#94a3b8" stroke="#334155" stroke-width="3"/><text x="407" y="353" font-family="sans-serif" font-size="16" font-weight="700" fill="#fff">BME280</text><g fill="#d6a84f"><circle cx="385" cy="390" r="6"/><circle cx="420" cy="390" r="6"/><circle cx="455" cy="390" r="6"/><circle cx="490" cy="390" r="6"/></g></g>',
            '<g stroke="#475569" stroke-width="6" stroke-linecap="round"><path d="M280 265h48M280 295h48M280 325h48M280 355h48"/></g><text x="280" y="390" font-family="sans-serif" font-size="12" fill="#334155">air vents in cube wall</text><text x="365" y="430" font-family="sans-serif" font-size="11" fill="#334155">VIN GND SCL SDA* header</text>'
        )
        route='<path d="M385 390L320 430L280 430M420 390L320 445L280 445" fill="none" stroke="#dc2626" stroke-width="4"/><path d="M455 390L315 460L280 460M490 390L310 475L280 475" fill="none" stroke="#2563eb" stroke-width="5"/><text x="545" y="462" font-family="sans-serif" font-size="12" fill="#1e3a8a">SDA / SCL</text>'
        focus='RIGHT FACE FOCUS: BME280 module and airflow openings.'
        actions=@(
            @('Mark RIGHT on the face layout and keep its sensor area exposed to air.','Do not put the sensor next to the ESP32 regulator or buzzer.','Check: there is a path for room air through the side wall.'),
            @('Place the actual BME280 breakout behind the planned vent area.','Trace its actual mounting hole and header pins from your module.','Check: sensor chip lines up with open vents and is not sealed in glue.'),
            @('Add the four-pin header and label VIN, GND, SCL and SDA.','Power it from 3.3 V only if your breakout accepts 3.3 V input.','Check: its SDA/SCL pull-ups will not pull ESP32 signals to 5 V.'),
            @('Route VIN red, GND black, and SDA/SCL blue to the internal shelf.','Share SDA/SCL with OLED by joining the same named wires.','Check: SDA goes to SDA and SCL goes to SCL.'),
            @('Solder each lead by its label and keep the vent area open.','Avoid excess glue or solder near the tiny sensor opening.','Check: module is secure, vents are clear, and no adjacent pins bridge.')
        )
    }
)

function MakeFaceStep($face, [int]$stepIndex) {
    $stageNames = @('Mark the face layout','Place the main module','Add header and remaining parts','Route labeled wires','Solder and inspect')
    $oldComponents = ''
    $newComponents = ''
    if ($stepIndex -ge 1) { $oldComponents = $face.parts[0] }
    if ($stepIndex -ge 2) { $newComponents = $face.parts[1] }
    $wireLayer = ''
    if ($stepIndex -ge 3) { $wireLayer = $face.route }
    $solderLayer = ''
    if ($stepIndex -eq 4) {
        $solderLayer = '<g fill="#fef08a" stroke="#b45309" stroke-width="2"><circle cx="290" cy="430" r="8"/><circle cx="290" cy="445" r="8"/><circle cx="290" cy="460" r="8"/></g><text x="545" y="505" font-family="sans-serif" font-size="12" fill="#854d0e">Highlighted circles = joints to inspect</text>'
    }
    $oldOpacity = if ($stepIndex -ge 2) { '.30' } else { '1' }
    $newOpacity = if ($stepIndex -ge 2) { '1' } else { '1' }
    $actions = $face.actions[$stepIndex]
    $stepTitle = 'Step ' + ($stepIndex + 1) + ': ' + $stageNames[$stepIndex]
    $svg = @"
<svg xmlns="http://www.w3.org/2000/svg" width="900" height="760" viewBox="0 0 900 760" role="img" aria-labelledby="title desc">
<title id="title">$($face.title) - $stepTitle</title><desc id="desc">$($face.subtitle) Flat dotted placement view for one cube face.</desc>
<defs><pattern id="dots" width="18" height="18" patternUnits="userSpaceOnUse"><circle cx="9" cy="9" r="3.4" fill="#64748b"/></pattern></defs>
<rect width="900" height="760" fill="#f8fafc"/><text x="35" y="42" font-family="sans-serif" font-size="25" font-weight="700" fill="#142033">$($face.title) · $stepTitle</text><text x="35" y="72" font-family="sans-serif" font-size="15" fill="#475569">$($face.subtitle)</text>
<rect x="175" y="118" width="550" height="340" rx="7" fill="#e7efe8" stroke="#334155" stroke-width="3"/><rect x="177" y="120" width="546" height="336" fill="url(#dots)"/><text x="190" y="108" font-family="sans-serif" font-size="12" font-weight="700" fill="#334155">FLAT FACE PLANNING BOARD · 2.54 mm hole spacing</text>
<g opacity="$oldOpacity">$oldComponents</g><g opacity="$newOpacity">$newComponents</g><g>$wireLayer</g><g>$solderLayer</g>
<rect x="35" y="492" width="830" height="208" rx="12" fill="#fff" stroke="#cbd5e1" stroke-width="2"/><text x="55" y="523" font-family="sans-serif" font-size="18" font-weight="700" fill="#142033">$($face.focus)</text><text x="55" y="558" font-family="sans-serif" font-size="14" font-weight="700" fill="#0f766e">TOP SIDE · DO</text><text x="180" y="558" font-family="sans-serif" font-size="14" fill="#334155">$(Xml $actions[0])</text><text x="55" y="591" font-family="sans-serif" font-size="14" font-weight="700" fill="#0f766e">BOTTOM SIDE · THEN</text><text x="220" y="591" font-family="sans-serif" font-size="14" fill="#334155">$(Xml $actions[1])</text><text x="55" y="632" font-family="sans-serif" font-size="14" font-weight="700" fill="#047857">CHECKPOINT</text><text x="180" y="632" font-family="sans-serif" font-size="14" fill="#047857">$(Xml $actions[2])</text>
<text x="55" y="672" font-family="sans-serif" font-size="12" fill="#64748b">Dotted background is a placement guide, not a cube wall. Mount the real component to the enclosure face.</text>
<g transform="translate(690 650)" font-family="sans-serif" font-size="11"><circle cx="0" cy="0" r="6" fill="#dc2626"/><text x="10" y="4" fill="#334155">3V3</text><circle cx="56" cy="0" r="6" fill="#111827"/><text x="66" y="4" fill="#334155">GND</text><circle cx="120" cy="0" r="6" fill="#2563eb"/><text x="130" y="4" fill="#334155">I2C</text></g>
</svg>
"@
    return $svg
}

# Remove only the old generated images from the workspace root. Preserve user images and source files.
Get-ChildItem -LiteralPath $root -File | Where-Object { $_.Name -match '^(step_\d+|face_(front|back|top|bottom|left|right)|deskpal_components)\.svg$' } | Remove-Item -Force
$faceKeys = @('front','back','top','bottom','left','right')
foreach ($key in $faceKeys) {
    $dir = Join-Path $root ('face_' + $key)
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    Get-ChildItem -LiteralPath $dir -File -Filter '*.svg' -ErrorAction SilentlyContinue | Remove-Item -Force
    $face = $faces | Where-Object key -eq $key
    for ($s=0; $s -lt 5; $s++) {
        $svg = MakeFaceStep $face $s
        $file = Join-Path $dir ('step_{0:D2}.svg' -f ($s + 1))
        [System.IO.File]::WriteAllText($file,$svg,[System.Text.UTF8Encoding]::new($false))
    }
}

$overviewDir = Join-Path $root 'overview'
New-Item -ItemType Directory -Path $overviewDir -Force | Out-Null
Get-ChildItem -LiteralPath $overviewDir -File -Filter '*.svg' -ErrorAction SilentlyContinue | Remove-Item -Force
$cubeSvg = @'
<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="720" viewBox="0 0 1000 720" role="img" aria-labelledby="t d"><title id="t">DeskPal cube assembly overview</title><desc id="d">Separate front, top, side and back components connect by wires to one flat internal perfboard on the bottom shelf.</desc><rect width="1000" height="720" fill="#f8fafc"/><text x="35" y="45" font-family="sans-serif" font-size="27" font-weight="700" fill="#142033">HOW SIX FACES JOIN AROUND ONE INTERNAL BOARD</text><text x="35" y="76" font-family="sans-serif" font-size="15" fill="#475569">The dotted face diagrams are flat planning views. The cube walls are separate enclosure panels.</text>
<polygon points="120,230 210,165 500,165 410,230" fill="#dbeafe" stroke="#334155" stroke-width="3"/><polygon points="410,230 500,165 500,485 410,550" fill="#cbd5e1" stroke="#334155" stroke-width="3"/><polygon points="120,230 410,230 410,550 120,550" fill="#eff6ff" stroke="#334155" stroke-width="3"/>
<rect x="205" y="330" width="120" height="95" rx="8" fill="#111827" stroke="#475569" stroke-width="4"/><text x="236" y="383" font-family="sans-serif" font-size="19" fill="#a7f3d0">OLED</text><circle cx="449" cy="302" r="25" fill="#111827" stroke="#475569" stroke-width="3"/><rect x="430" y="370" width="38" height="49" rx="4" fill="#15803d" stroke="#166534" stroke-width="2"/><g><circle cx="263" cy="203" r="15" fill="#d6a84f" stroke="#fef3c7" stroke-width="3"/><circle cx="320" cy="203" r="12" fill="#dc2626" stroke="#991b1b" stroke-width="2"/><rect x="360" y="190" width="36" height="19" rx="4" fill="#94a3b8" stroke="#475569" stroke-width="2"/></g>
<polygon points="170,505 350,505 385,478 205,478" fill="#d1fae5" stroke="#047857" stroke-width="3"/><polygon points="225,493 300,493 320,478 245,478" fill="#14532d" stroke="#064e3b" stroke-width="2"/><text x="190" y="527" font-family="sans-serif" font-size="12" fill="#065f46">ONE FLAT PERFBOARD SHELF</text><rect x="284" y="510" width="47" height="24" rx="9" fill="#fef3c7" stroke="#b45309"/><text x="286" y="526" font-family="sans-serif" font-size="9" fill="#78350f">LiPo</text>
<path d="M292 491L258 440L265 424" fill="none" stroke="#2563eb" stroke-width="5"/><path d="M300 493L449 338" fill="none" stroke="#2563eb" stroke-width="5"/><path d="M250 490L290 230" fill="none" stroke="#16a34a" stroke-width="5"/><path d="M235 491L450 390" fill="none" stroke="#eab308" stroke-width="5"/><path d="M321 510L424 452" fill="none" stroke="#dc2626" stroke-width="5"/><path d="M329 518L423 466" fill="none" stroke="#111827" stroke-width="5"/>
<text x="145" y="590" font-family="sans-serif" font-size="13" fill="#334155">FRONT: OLED</text><text x="245" y="140" font-family="sans-serif" font-size="13" fill="#334155">TOP: touch + key + quiet switch</text><text x="510" y="322" font-family="sans-serif" font-size="13" fill="#334155">RIGHT: BME280</text><text x="510" y="343" font-family="sans-serif" font-size="13" fill="#334155">LEFT: round mic (opposite side)</text><text x="505" y="480" font-family="sans-serif" font-size="13" fill="#334155">BACK: charger USB-C + ESP32 USB-C</text><text x="505" y="502" font-family="sans-serif" font-size="13" fill="#334155">BOTTOM: feet + standoffs support shelf</text><text x="505" y="544" font-family="sans-serif" font-size="13" fill="#334155">All face wires pass inward to the shelf.</text>
<g transform="translate(530 150)" font-family="sans-serif" font-size="13"><text x="0" y="0" font-weight="700" fill="#142033">WIRE COLOR KEY</text><path d="M0 26h38" stroke="#dc2626" stroke-width="6"/><text x="48" y="31" fill="#334155">regulated 3.3V</text><path d="M0 52h38" stroke="#111827" stroke-width="6"/><text x="48" y="57" fill="#334155">GND return</text><path d="M0 78h38" stroke="#2563eb" stroke-width="6"/><text x="48" y="83" fill="#334155">I2C SDA/SCL</text><path d="M0 104h38" stroke="#eab308" stroke-width="6"/><text x="48" y="109" fill="#334155">mic audio wires</text><path d="M0 130h38" stroke="#16a34a" stroke-width="6"/><text x="48" y="135" fill="#334155">button/touch signals</text></g>
<rect x="35" y="630" width="930" height="58" rx="10" fill="#fffbeb" stroke="#f59e0b" stroke-width="2"/><text x="55" y="655" font-family="sans-serif" font-size="13" font-weight="700" fill="#78350f">IMPORTANT:</text><text x="145" y="655" font-family="sans-serif" font-size="13" fill="#78350f">SPDT selects quiet mode at GPIO26; use a separate way to disconnect load while charging.</text><text x="55" y="677" font-family="sans-serif" font-size="12" fill="#78350f">Do not connect raw LiPo voltage to ESP32 3V3/5V; check charger current against the cell specification.</text></svg>
'@
[System.IO.File]::WriteAllText((Join-Path $overviewDir 'cube_assembly.svg'),$cubeSvg,[System.Text.UTF8Encoding]::new($false))

$wireSvg = @'
<svg xmlns="http://www.w3.org/2000/svg" width="1100" height="760" viewBox="0 0 1100 760" role="img" aria-labelledby="wt wd"><title id="wt">DeskPal face-to-face wiring map</title><desc id="wd">Each wall module connects by labeled wire through its face to the ESP32 on the internal flat perfboard shelf.</desc><rect width="1100" height="760" fill="#f8fafc"/><text x="35" y="43" font-family="sans-serif" font-size="27" font-weight="700" fill="#142033">FACE WIRES → INTERNAL PERFBOARD</text><text x="35" y="73" font-family="sans-serif" font-size="15" fill="#475569">Schematic only: route insulated wires through each wall and solder to matching labeled points.</text>
<rect x="386" y="280" width="330" height="310" rx="16" fill="#d1fae5" stroke="#047857" stroke-width="3"/><text x="414" y="311" font-family="sans-serif" font-size="18" font-weight="700" fill="#065f46">ONE FLAT INTERNAL SHELF</text><rect x="464" y="354" width="175" height="122" rx="12" fill="#14532d" stroke="#064e3b" stroke-width="3"/><rect x="495" y="386" width="112" height="55" rx="6" fill="#94a3b8" stroke="#e2e8f0" stroke-width="3"/><text x="510" y="418" font-family="sans-serif" font-size="13" fill="#fff">CLASSIC ESP32</text><text x="449" y="497" font-family="sans-serif" font-size="12" fill="#334155">GPIO map below follows DeskPalConfig.h</text>
<g font-family="sans-serif"><rect x="45" y="130" width="245" height="120" rx="12" fill="#fff" stroke="#2563eb" stroke-width="3"/><text x="65" y="160" font-size="16" font-weight="700" fill="#142033">FRONT · OLED</text><text x="65" y="190" font-size="13" fill="#334155">SDA / SCL / 3V3 / GND</text><text x="65" y="216" font-size="12" fill="#64748b">screen faces out</text>
<rect x="420" y="112" width="285" height="128" rx="12" fill="#fff" stroke="#16a34a" stroke-width="3"/><text x="440" y="143" font-size="16" font-weight="700" fill="#142033">TOP · TOUCH + BUTTON + SPDT</text><text x="440" y="174" font-size="13" fill="#334155">OUT GPIO32 · button GPIO25</text><text x="440" y="200" font-size="13" fill="#334155">quiet switch GPIO26 → GND</text>
<rect x="810" y="145" width="245" height="120" rx="12" fill="#fff" stroke="#2563eb" stroke-width="3"/><text x="830" y="176" font-size="16" font-weight="700" fill="#142033">RIGHT · BME280</text><text x="830" y="205" font-size="13" fill="#334155">SDA/SCL shared with OLED</text><text x="830" y="231" font-size="12" fill="#64748b">keep air vents open</text>
<rect x="45" y="365" width="250" height="145" rx="12" fill="#fff" stroke="#eab308" stroke-width="3"/><text x="65" y="397" font-size="16" font-weight="700" fill="#142033">LEFT · INMP441 MIC</text><text x="65" y="427" font-size="13" fill="#334155">SCK 18 · WS 19 · SD 35</text><text x="65" y="453" font-size="13" fill="#334155">VDD 3V3 · L/R GND*</text><text x="65" y="480" font-size="12" fill="#64748b">*check breakout labels</text>
<rect x="810" y="355" width="245" height="150" rx="12" fill="#fff" stroke="#7c3aed" stroke-width="3"/><text x="830" y="388" font-size="16" font-weight="700" fill="#142033">BACK · TP4056</text><text x="830" y="418" font-size="13" fill="#334155">B+ / B− → LiPo</text><text x="830" y="444" font-size="13" fill="#334155">OUT → disconnect → regulator</text><text x="830" y="472" font-size="12" fill="#64748b">not the quiet-mode switch</text>
<rect x="400" y="635" width="300" height="72" rx="12" fill="#fff" stroke="#64748b" stroke-width="2"/><text x="420" y="665" font-size="15" font-weight="700" fill="#142033">BUZZER inside: GPIO27 via driver if needed</text><text x="420" y="690" font-size="12" fill="#64748b">Check buzzer voltage/current and polarity first.</text></g>
<path d="M290 190L380 300L495 360M290 213L365 318L495 382" fill="none" stroke="#2563eb" stroke-width="5"/><path d="M290 230L370 335L485 400" fill="none" stroke="#dc2626" stroke-width="5"/><path d="M290 242L374 348L490 418" fill="none" stroke="#111827" stroke-width="5"/>
<path d="M560 240L560 330L560 354" fill="none" stroke="#16a34a" stroke-width="5"/><path d="M705 202L775 310L810 390M705 220L780 330L810 418" fill="none" stroke="#2563eb" stroke-width="5"/>
<path d="M295 408L350 420L386 430M295 434L355 443L386 452M295 460L360 465L386 474" fill="none" stroke="#eab308" stroke-width="5"/><path d="M705 470L755 470L810 465M705 490L755 490L810 482" fill="none" stroke="#dc2626" stroke-width="5"/><path d="M560 590L560 635" stroke="#16a34a" stroke-width="5"/>
<g font-family="sans-serif" font-size="13"><text x="760" y="565" fill="#dc2626">RED = regulated 3.3 V</text><text x="760" y="589" fill="#111827">BLACK = GND (all grounds join)</text><text x="760" y="613" fill="#2563eb">BLUE = I2C SDA/SCL</text><text x="760" y="637" fill="#854d0e">YELLOW = mic audio</text><text x="760" y="661" fill="#166534">GREEN = touch/button controls</text><text x="760" y="685" fill="#7c3aed">PURPLE = quiet-mode switch</text></g>
<text x="35" y="730" font-family="sans-serif" font-size="13" font-weight="700" fill="#991b1b">Never connect raw LiPo voltage to ESP32 3V3/5V. Disconnect the load while charging; 500 mAh is not a charge-current rating.</text></svg>
'@
[System.IO.File]::WriteAllText((Join-Path $overviewDir 'wiring_map.svg'),$wireSvg,[System.Text.UTF8Encoding]::new($false))

$overviewMd = @'
# Overview: how the faces connect

The front, back, top, left and right are separate enclosure walls. The bottom wall carries feet and standoffs. One flat perfboard is held horizontally above the bottom wall as the internal shelf. The perfboard does not become any of the walls.

Each face diagram is a flat 2D placement view. Mount the actual module to its wall, pass flexible labeled wires through a smooth opening, and solder the wire ends to the matching rows or connectors on the internal shelf. Keep a small service loop so the walls can open.

## Connection map

| From | To | Wire / note |
|---|---|---|
| OLED SDA and BME280 SDA | ESP32 GPIO21 | Blue I2C data; join SDA only to SDA |
| OLED SCL and BME280 SCL | ESP32 GPIO22 | Blue I2C clock; join SCL only to SCL |
| OLED/BME280 confirmed 3.3 V supply | Regulated 3.3 V row | Red; do not assume a 5 V labeled board is 3.3 V logic-safe |
| INMP441 SCK / WS / SD | GPIO18 / GPIO19 / GPIO35 | Yellow audio bundle; matches DeskPalConfig.h |
| INMP441 L/R | GND | Select left channel if the breakout label agrees |
| TTP223 OUT | GPIO32 | Green control signal |
| Momentary tactile button | GPIO25 and GND | Firmware uses `INPUT_PULLUP`; this is the mic-mute button |
| SPDT switch | GPIO26 common; one outer pin to GND | Quiet-mode input; leave the other outer pin open |
| ESP32 GPIO27 | Active buzzer through suitable driver if required | Check buzzer rated voltage/current and polarity |
| TP4056 B+ / B− | LiPo + / − | Confirm the actual board markings and polarity |
| TP4056 OUT+ / OUT− (if present) | Separate load disconnect/regulator input and common GND | Use protected output as the module instructions say |

Every module ground, ESP32 ground, and regulator ground must be connected together. Power and ground wires are not the same as signal wires. GPIO35 is input-only and is used for mic data. GPIO34 is reserved for optional battery measurement, which is disabled; never connect a battery directly to a GPIO.

## Safe power-up order

1. Build and inspect with the battery disconnected.
2. Check the ground and signal wires using a multimeter.
3. Power the ESP32 from USB alone for the first test.
4. Add one face module at a time, removing power before each wiring change.
5. Use the battery only after the regulator output and TP4056 current setting have been verified for the 500 mAh cell.
6. Disconnect the load while charging with the TP4056. The SPDT switch is a quiet-mode input, not a load disconnect; add a separate disconnect or unplug the load.

The 500 mAh number is cell capacity, not its safe charge-current setting. Follow the battery maker's charge-current limit. A “1 A” charger board may need its current reduced before use with this cell.
'@
[System.IO.File]::WriteAllText((Join-Path $overviewDir 'README.md'),$overviewMd,[System.Text.UTF8Encoding]::new($true))
