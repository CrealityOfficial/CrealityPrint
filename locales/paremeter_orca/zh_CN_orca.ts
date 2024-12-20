<?xml version='1.0'?>
<TS version="2.1" language="zh_CN">
    <context>
        <name>ParameterComponent</name>
	<message>
        <source>After a tool change, the exact position of the newly loaded filament inside the nozzle may not be known, and the filament pressure is likely not yet stable. Before purging the print head into an infill or a sacrificial object, Orca Slicer will always prime this amount of material into the wipe tower to produce successive infill or sacrificial object extrusions reliably.</source>
        <translation>换色后，新加载的耗材在喷嘴内的确切位置可能未知，耗材压力可能还不稳定。在冲刷打印头到填充或作为挤出废料之前，Orca Slicer将始终将这些的耗材丝冲刷到擦拭塔中以产生连续的填充或稳定的挤出废料。</translation>
    </message>
		<message>
        <source>Discribe how long the nozzle will move along the last path when retracting.

Depending on how long the wipe operation lasts, how fast and long the extruder/filament retraction settings are, a retraction move may be needed to retract the remaining filament.

Setting a value in the retract amount before wipe setting below will perform any excess retraction before the wipe, else it will be performed after.</source>
        <translation>喷嘴在回抽时将沿着最后路径移动的距离

根据擦拭操作的距离以及挤出机/耗材丝回抽的速度和长度，可能需要执行额外的回抽动作以收回剩余的丝材。

在下方的擦拭前回抽量设置中输入一个数值，将在擦拭动作之前执行任何超出部分的回抽，否则超出部分的回抽将在擦拭之后执行。</translation>
    </message>
        <message>
            <source>Print order within a single layer</source>
            <translation>同一层内的打印顺序</translation>
        </message>
        <message>
            <source>Number of bottom interface layers</source>
            <translation>底部界面层的数量</translation>
        </message>
        <message>
            <source>Extrude perimeters that have a part over an overhang in the reverse direction on odd layers. This alternating pattern can drastically improve steep overhangs.

This setting can also help reduce part warping due to the reduction of stresses in the part walls.</source>
        <translation>在奇数层上，将悬垂部分的走线反转。这种交替的走线模式可以大大改善陡峭的悬垂。

这个设置也可以帮助减少零件变形，因为零件墙壁的应力减少了。</translation>
    </message>
	    <message>
        <source>This option creates bridges for counterbore holes, allowing them to be printed without support. Available modes include:
1. None: No bridge is created.
2. Partially Bridged: Only a part of the unsupported area will be bridged.
3. Sacrificial Layer: A full sacrificial bridge layer is created.</source>
        <translation>此选项为沉孔创建搭桥，使其可以在无支撑的情况下打印。可用的模式包括：
1. 无：不创建搭桥。
2. 部分搭桥：仅部分不支撑的区域将被搭桥。
3. 牺牲层：创建一个完整的牺牲层。</translation>
    </message>
	<message>
        <source>This option can help reducing pillowing on top surfaces in heavily slanted or curved models.

By default, small internal bridges are filtered out and the internal solid infill is printed directly over the sparse infill. This works well in most cases, speeding up printing without too much compromise on top surface quality.

However, in heavily slanted or curved models especially where too low sparse infill density is used, this may result in curling of the unsupported solid infill, causing pillowing.

Enabling this option will print internal bridge layer over slightly unsupported internal solid infill. The options below control the amount of filtering, i.e. the amount of internal bridges created.

Disabled - Disables this option. This is the default behaviour and works well in most cases.

Limited filtering - Creates internal bridges on heavily slanted surfaces, while avoiding creating uncessesary interal bridges. This works well for most difficult models.

No filtering - Creates internal bridges on every potential internal overhang. This option is useful for heavily slanted top surface models. However, in most cases it creates too many unecessary bridges.</source>
        <translation>此选项可以帮助减少在严重倾斜或弯曲模型的顶部表面上的枕头现象。

默认情况下，小的内部搭桥会被过滤掉，内部实心填充会直接打印在稀疏填充上。这在大多数情况下效果很好，可以加快打印速度，而对顶部表面质量的影响不大。

然而，在严重倾斜或弯曲的模型中，特别是在使用了过低的稀疏填充密度的情况下，这可能会导致不支撑的实心填充卷曲，从而导致枕头现象。

启用此选项将在轻微不支撑的内部实心填充上打印内部搭桥层。下面的选项控制过滤的程度，即创建的内部搭桥的数量。

禁用 - 禁用此选项。这是默认行为，在大多数情况下效果很好。

有限过滤 - 在严重倾斜的表面上创建内部搭桥，同时避免创建不必要的内部搭桥。这对大多数困难模型效果很好。

无过滤 - 在每个潜在的内部悬垂上创建内部搭桥。这个选项对于严重倾斜的顶部表面模型很有用。然而，在大多数情况下，它会创建太多不必要的桥接。</translation>
    </message>
	    <message>
        <source>Flow Compensation Model, used to adjust the flow for small infill areas. The model is expressed as a comma separated pair of values for extrusion length and flow correction factors, one per line, in the following format: &quot;1.234,5.678&quot;</source>
        <translation>流量补偿模型，用于调整小区域填充的流量。模型以逗号分隔的形式表示，每行一个值，格式如下：&quot;1.234,5.678&quot;</translation>
    </message>
	    <message>
        <source>Enable flow compensation for small infill areas</source>
        <translation>启用小区域填充的流量补偿</translation>
    </message>
   <message>
        <source>If a top surface has to be printed and it&apos;s partially covered by another layer, it won&apos;t be considered at a top layer where its width is below this value. This can be useful to not let the &apos;one perimeter on top&apos; trigger on surface that should be covered only by perimeters. This value can be a mm or a % of the perimeter extrusion width.
Warning: If enabled, artifacts can be created if you have some thin features on the next layer, like letters. Set this setting to 0 to remove these artifacts.</source>
        <translation>如果顶面需要打印，但是它的一部分被其它层覆盖，那么当它的宽度小于这个值时，它不会被认为是顶层。这个设置可以用于避免在狭窄顶面上触发“顶面单层墙”。这个值可以是毫米或线宽的百分比。
警告：如果启用，可能会在下一层上产生一些薄的特征，比如字母。将此设置设置为0可以消除这些伪影。</translation>
        <extra-po-flags>no-c-format, no-boost-format</extra-po-flags>
    </message>
	    <message>
        <source>The direction which the wall loops are extruded when looking down from the top.

By default all walls are extruded in counter-clockwise, unless Reverse on odd is enabled. Set this to any option other than Auto will force the wall direction regardless of the Reverse on odd.

This option will be disabled if sprial vase mode is enabled.</source>
        <translation>从顶部往下看时,墙壁被打印的方向。

默认情况下,所有墙壁都按逆时针方向被打印,除非启用了奇数层翻转选项。将此选项设置为除自动之外的任何选项,都会强制指定墙壁方向,而不受奇数层翻转选项的影响。

如果启用了螺旋花瓶模式,此选项将被禁用。</translation>
    </message>
	    <message>
        <source>Adjust this value to prevent short, unclosed walls from being printed, which could increase print time. Higher values remove more and longer walls.

NOTE: Bottom and top surfaces will not be affected by this value to prevent visual gaps on the ouside of the model. Adjust &apos;One wall threshold&apos; in the Advanced settings below to adjust the sensitivity of what is considered a top-surface. &apos;One wall threshold&apos; is only visibile if this setting is set above the default value of 0.5, or if single-wall top surfaces is enabled.</source>
        <translation>调整这个值以省略打印短的、未闭合的墙，这些可能会增加打印时间。设置较高的值将移除更多和更长的墙。

注意：底部和顶部表面不会受到这个值的影响，以防止模型外部出现肉眼可见间隙。调整下面的高级设置中的“单层墙阈值”来调整什么被认为是顶部表面的敏感度。只有当这个设置高于默认值0.5，或者启用了单层顶部表面时，“单层墙阈值”才会显示。</translation>
    </message>
	   <message>
      <source>Use scarf joint to minimize seam visibility and increase seam strength.</source>
      <translation>使用斜拼接缝来最小化接缝的可见性并增加接缝的强度。</translation>
    </message>
	    <message>
      <source>Apply scarf joints only to smooth perimeters where traditional seams do not conceal the seams at sharp corners effectively.</source>
      <translation>当墙壁没有合适的锐角以至于传统接缝无法有效隐藏的时候使用斜拼接缝。</translation>
    </message>
	    <message>
        <source>This option sets the printing speed for scarf joints. It is recommended to print scarf joints at a slow speed (less than 100 mm/s).  It&apos;s also advisable to enable &apos;Extrusion rate smoothing&apos; if the set speed varies significantly from the speed of the outer or inner walls. If the speed specified here is higher than the speed of the outer or inner walls, the printer will default to the slower of the two speeds. When specified as a percentage (e.g., 80%), the speed is calculated based on the respective outer or inner wall speed. The default value is set to 100%.</source>
        <translation>这个选项设置斜拼接缝的打印速度。建议以较慢的速度（小于100mm/s）打印斜拼接缝。如果设置的速度与外墙或内墙的速度相差较大，建议启用“平滑挤出率”。如果此处指定的速度高于外墙或内墙的速度，则打印机将默认使用两者中较慢的速度。当以百分比（例如80%）指定时，速度将基于外墙或内墙的速度进行计算。默认值为100%。</translation>
    </message>
    <message>
        <source>This option sets the threshold angle for applying a conditional scarf joint seam.
If the maximum angle within the perimeter loop exceeds this value (indicating the absence of sharp corners), a scarf joint seam will be used. The default value is 155°.</source>
        <translation>此选项设置判断是否应用斜拼接缝的角度阈值。
如果围墙环内的最大角度超过了这个值（表示没有足够锐的角），则使用斜拼接缝接缝。默认值为155°。</translation>
    </message>
	<message>
        <source>Start height of the scarf.
This amount can be specified in millimeters or as a percentage of the current layer height. The default value for this parameter is 0.</source>
        <translation>斜拼接缝的起始高度。
这个数值可以用毫米或者当前层高的百分比表示。默认值为0。</translation>
    </message>
	    <message>
      <source>The scarf extends to the entire length of the wall.</source>
      <translation>将斜拼接缝延伸到整个围墙</translation>
    </message>
	    <message>
      <source>Length of the scarf. Setting this parameter to zero effectively disables the scarf.</source>
      <translation>斜拼接缝的长度。长度为0时会禁用斜拼接缝</translation>
    </message>
	    <message>
      <source>Minimum number of segments of each scarf.</source>
      <translation>斜拼接缝所需的最少段数</translation>
    </message>
	    <message>
      <source>Use scarf joint for inner walls as well.</source>
      <translation>同时应用斜拼接缝于内墙</translation>
    </message>
	    <message>
    <source>The wipe speed is determined by the speed of the current extrusion role.e.g. if a wipe action is executed immediately following an outer wall extrusion, the speed of the outer wall extrusion will be utilized for the wipe action.</source>
      <translation>擦拭速度由当前挤出类型的速度决定。例如，如果擦拭动作紧随外墙，擦拭速度将使用外墙的速度。</translation>
    </message>
    <message>
      <source>Quality</source>
      <translation>质量</translation>
    </message>
    <message>
      <source>Layer Height</source>
      <translation>层高</translation>
    </message>
    <message>
      <source>Line Width</source>
      <translation>线宽</translation>
    </message>
    <message>
      <source>Seam</source>
      <translation>接缝</translation>
    </message>
    <message>
      <source>Precision</source>
      <translation>精度</translation>
    </message>
    <message>
      <source>Ironing</source>
      <translation>熨烫</translation>
    </message>
    <message>
      <source>Wall generator</source>
      <translation>墙生成器</translation>
    </message>
    <message>
      <source>Advanced</source>
      <translation>高级</translation>
    </message>
    <message>
      <source>Strength</source>
      <translation>强度</translation>
    </message>
    <message>
      <source>Top/bottom shells</source>
      <translation>顶部/底部外壳</translation>
    </message>
    <message>
      <source>Infill</source>
      <translation>填充</translation>
    </message>
    <message>
      <source>Speed</source>
      <translation>速度</translation>
    </message>
    <message>
      <source>Overhang speed</source>
      <translation>悬垂速度</translation>
    </message>
    <message>
      <source>Travel speed</source>
      <translation>空驶速度</translation>
    </message>
    <message>
      <source>Acceleration</source>
      <translation>加速度</translation>
    </message>
    <message>
      <source>Jerk(XY)</source>
      <translation>抖动（XY轴）</translation>
    </message>
    <message>
      <source>Support</source>
      <translation>支撑</translation>
    </message>
    <message>
      <source>Raft</source>
      <translation>筏层</translation>
    </message>
    <message>
      <source>Other</source>
      <translation>其他</translation>
    </message>
    <message>
      <source>Bed adhension</source>
      <translation>热床粘接</translation>
    </message>
    <message>
      <source>Prime Tower</source>
      <translation>擦拭塔</translation>
    </message>
    <message>
      <source>Flush Options</source>
      <translation>换料冲刷选项</translation>
    </message>
    <message>
      <source>G-code output</source>
      <translation>G-code 输出</translation>
    </message>
    <message>
      <source>Filament</source>
      <translation>耗材丝</translation>
    </message>
    <message>
      <source>Recommended nozzle temperature</source>
      <translation>建议喷嘴温度</translation>
    </message>
    <message>
      <source>Print chamber temperature</source>
      <translation>打印仓温度</translation>
    </message>
    <message>
      <source>Print temperature</source>
      <translation>打印温度</translation>
    </message>
    <message>
      <source>Nozzle</source>
      <translation>喷嘴</translation>
    </message>
    <message>
      <source>Bed temperature</source>
      <translation>床温</translation>
    </message>
    <message>
      <source>Cool plate</source>
      <translation>低温打印热床</translation>
    </message>
    <message>
      <source>Engineering plate</source>
      <translation>工程材料热床</translation>
    </message>
    <message>
      <source>Smooth PEI Plate / High Temp Plate</source>
      <translation>光滑PEI板</translation>
    </message>
    <message>
      <source>Textured PEI Plate</source>
      <translation>纹理PEI板</translation>
    </message>
    <message>
      <source>Volumetric speed limitation</source>
      <translation>体积速度限制</translation>
    </message>
    <message>
      <source>Cooling for specific layer</source>
      <translation>特定层冷却</translation>
    </message>
    <message>
      <source>Part cooling fan</source>
      <translation>部件冷却风扇</translation>
    </message>
    <message>
      <source>Min fan speed threshold</source>
      <translation>最小风扇速度阈值</translation>
    </message>
    <message>
      <source>Max fan speed threshold</source>
      <translation>最大风扇速度阈值</translation>
    </message>
    <message>
      <source>Auxiliary part cooling fan</source>
      <translation>辅助部件冷却风扇</translation>
    </message>
    <message>
      <source>Exhaust fan</source>
      <translation>排气风扇</translation>
    </message>
    <message>
      <source>Setting Overrides</source>
      <translation>参数覆盖</translation>
    </message>
    <message>
      <source>Retraction</source>
      <translation>回抽</translation>
    </message>
    <message>
      <source>Multimaterial</source>
      <translation>材料</translation>
    </message>
    <message>
      <source>Wipe tower parameters</source>
      <translation>色塔参数</translation>
    </message>
    <message>
      <source>Toolchange parameters with single extruder MM printers</source>
      <translation>单挤出机多材料打印机的换色参数</translation>
    </message>
    <message>
      <source>Basic information</source>
      <translation>基础信息</translation>
    </message>
    <message>
      <source>Printable space</source>
      <translation>可打印区域</translation>
    </message>
    <message>
      <source>Cooling Fan</source>
      <translation>冷却风扇</translation>
    </message>
    <message>
      <source>Extruder Clearance</source>
      <translation>挤出机避让空间</translation>
    </message>
    <message>
      <source>Accessory</source>
      <translation>配件</translation>
    </message>
    <message>
      <source>Single extruder multimaterial setup</source>
      <translation>设置单挤出机多材料</translation>
    </message>
    <message>
      <source>Wipe tower</source>
      <translation>色塔</translation>
    </message>
    <message>
      <source>Single extruder multimaterial parameters</source>
      <translation>单挤出机多材料参数</translation>
    </message>
    <message>
      <source>Extruder</source>
      <translation>挤出机</translation>
    </message>
    <message>
      <source>Size</source>
      <translation>大小</translation>
    </message>
    <message>
      <source>Layer height limits</source>
      <translation>层高限制</translation>
    </message>
    <message>
      <source>Position</source>
      <translation>位置</translation>
    </message>
    <message>
      <source>Lift Z Enforcement</source>
      <translation>强化抬Z策略</translation>
    </message>
    <message>
      <source>Retraction when switching material</source>
      <translation>切换材料时的回抽量</translation>
    </message>
    <message>
      <source>Motion ability</source>
      <translation>移动能力</translation>
    </message>
    <message>
      <source>Speed limitation</source>
      <translation>速度限制</translation>
    </message>
    <message>
      <source>Acceleration limitation</source>
      <translation>加速度限制</translation>
    </message>
    <message>
      <source>Jerk limitation</source>
      <translation>抖动限制</translation>
    </message>
    <message>
      <source>Enable accel_to_decel</source>
      <translation>启用制动速度</translation>
    </message>
    <message>
      <source>Klipper's max_accel_to_decel will be adjusted automatically</source>
      <translation>Klipper的max_accel_to_decel将被自动调整</translation>
    </message>
    <message>
      <source>accel_to_decel</source>
      <translation>制动速度</translation>
    </message>
    <message>
      <source>Klipper's max_accel_to_decel will be adjusted to this %% of acceleration</source>
      <translation>Klipper的max_accel_to_decel将被调整为该加速度的百分比</translation>
    </message>
    <message>
      <source>Activate air filtration</source>
      <translation>启用空气过滤/排气</translation>
    </message>
    <message>
      <source>Activate for better air filtration. G-code command: M106 P3 S(0-255)</source>
      <translation>启用空气过滤/排气风扇。G-code命令：M106 P3 S(0-255)</translation>
    </message>
    <message>
      <source>Activate temperature control</source>
      <translation>激活温度控制</translation>
    </message>
    <message>
      <source>Enable this option for chamber temperature control. An M191 command will be added before &quot;machine_start_gcode&quot;
G-code commands: M141/M191 S(0-255)</source>
      <translation></translation>
    </message>
    <message>
      <source>Fan speed</source>
      <translation>风扇速度</translation>
    </message>
    <message>
      <source>Speed of auxiliary part cooling fan. Auxiliary fan will run at this speed during printing except the first several layers which is defined by no cooling layers.
Please enable auxiliary_fan in printer settings to use this feature. G-code command: M106 P2 S(0-255)</source>
      <translation>辅助部件冷却风扇的转速。辅助部件冷却风扇将一直运行在该速度，除了设置的无需冷却的前若干层
请在打印机设置中启用辅助风扇以使用此功能。G-code指令：M106 P2 S(0-255)</translation>
    </message>
    <message>
      <source>Enable this option if machine has auxiliary part cooling fan. G-code command: M106 P2 S(0-255).</source>
      <translation>如果打印机有辅助风扇，可以开启此选项。G-code指令：M106 P2 S(0-255)</translation>
    </message>
    <message>
      <source>Show auto-calibration marks</source>
      <translation>显示雷达标定线</translation>
    </message>
    <message>
      <source>Bed custom model</source>
      <translation>自定义热床模型</translation>
    </message>
    <message>
      <source>Bed custom texture</source>
      <translation>自定义热床纹理</translation>
    </message>
    <message>
      <source>Bed exclude area</source>
      <translation>不可打印区域</translation>
    </message>
    <message>
      <source>Unprintable area in XY plane. For example, X1 Series printers use the front left corner to cut filament during filament change. The area is expressed as polygon by points in following format: &quot;XxY, XxY, ...&quot;</source>
      <translation>XY平面上的不可打印区域。例如，X1系列打印机在换料过程中，会使用左前角区域来切断耗材丝。这个多边形区域由以下格式的点表示：“XxY，XxY，…”</translation>
    </message>
    <message>
      <source>Before layer change G-code</source>
      <translation>换层前G-code</translation>
    </message>
    <message>
      <source>This G-code is inserted at every layer change before lifting z</source>
      <translation>在每次换层抬升z高度之前插入这段G-code</translation>
    </message>
    <message>
      <source>Best object position</source>
      <translation>最佳对象位置</translation>
    </message>
    <message>
      <source>Best auto arranging position in range [0,1] w.r.t. bed shape.</source>
      <translation>最佳自动排列位置在[0,1]范围内，相对于打印床形状。</translation>
    </message>
    <message>
      <source>Bottom shell layers</source>
      <translation>底部壳体层数</translation>
    </message>
    <message>
      <source>This is the number of solid layers of bottom shell, including the bottom surface layer. When the thickness calculated by this value is thinner than bottom shell thickness, the bottom shell layers will be increased</source>
      <translation>底部壳体实心层层数，包括底面。当由该层数计算的厚度小于底部壳体厚度，切片时会增加底部壳体的层数</translation>
    </message>
    <message>
      <source>Bottom shell thickness</source>
      <translation>底部壳体厚度</translation>
    </message>
    <message>
      <source>The number of bottom solid layers is increased when slicing if the thickness calculated by bottom shell layers is thinner than this value. This can avoid having too thin shell when layer height is small. 0 means that this setting is disabled and thickness of bottom shell is absolutely determained by bottom shell layers</source>
      <translation>如果由底部壳体层数算出的厚度小于这个数值，那么切片时将自动增加底部壳体层数。这能够避免当层高很小时，底部壳体过薄。0表示关闭这个设置，同时底部壳体的厚度完全由底部壳体层数决定</translation>
    </message>
    <message>
      <source>Bottom surface flow ratio</source>
      <translation>底部表面流量比例</translation>
    </message>
    <message>
      <source>This factor affects the amount of material for bottom solid infill</source>
      <translation>首层流量调整系数，默认为1.0</translation>
    </message>
    <message>
      <source>Bottom surface pattern</source>
      <translation>底面图案</translation>
    </message>
    <message>
      <source>Line pattern of bottom surface infill, not bridge infill</source>
      <translation>除了桥接外的底面填充的走线图案</translation>
    </message>
    <message>
      <source>Aligned Rectilinear</source>
      <translation>直线排列</translation>
    </message>
    <message>
      <source>Archimedean Chords</source>
      <translation>阿基米德和弦</translation>
    </message>
    <message>
      <source>Concentric</source>
      <translation>同心</translation>
    </message>
    <message>
      <source>Hilbert Curve</source>
      <translation>希尔伯特曲线</translation>
    </message>
    <message>
      <source>Monotonic</source>
      <translation>单调</translation>
    </message>
    <message>
      <source>Monotonic line</source>
      <translation>单调线</translation>
    </message>
    <message>
      <source>Octagram Spiral</source>
      <translation>八角螺旋</translation>
    </message>
    <message>
      <source>Rectilinear</source>
      <translation>直线</translation>
    </message>
    <message>
      <source>Bridge</source>
      <translation>桥接</translation>
    </message>
    <message>
      <source>Acceleration of bridges. If the value is expressed as a percentage (e.g. 50%), it will be calculated based on the outer wall acceleration.</source>
      <translation>桥接加速度。如果该值以百分比（例如50%）表示，则将根据外墙加速度进行计算。</translation>
    </message>
    <message>
      <source>Bridge infill direction</source>
      <translation>拉桥填充方向</translation>
    </message>
    <message>
      <source>Bridge density</source>
      <translation>搭桥密度</translation>
    </message>
    <message>
      <source>Density of external bridges. 100% means solid bridge. Default is 100%.</source>
      <translation>外部桥接的密度。100%意味着实心桥。默认值为100%。</translation>
    </message>
    <message>
      <source>Bridge flow ratio</source>
      <translation>桥接流量</translation>
    </message>
    <message>
      <source>Decrease this value slightly(for example 0.9) to reduce the amount of material for bridge, to improve sag</source>
      <translation>稍微减小这个数值（比如0.9）可以减小桥接的材料量，来改善下垂。</translation>
    </message>
    <message>
      <source>Don't support bridges</source>
      <translation>不支撑桥接</translation>
    </message>
    <message>
      <source>Don't support the whole bridge area which make support very large. Bridge usually can be printing directly without support if not very long</source>
      <translation>不对整个桥接面进行支撑，否则支撑体会很大。不是很长的桥接通常可以无支撑直接打印。</translation>
    </message>
    <message>
      <source>External</source>
      <translation>外部</translation>
    </message>
    <message>
      <source>Speed of bridge and completely overhang wall</source>
      <translation>桥接和完全悬空的外墙的打印速度</translation>
    </message>
    <message>
      <source>Brim ears</source>
      <translation>圆盘</translation>
    </message>
    <message>
      <source>Only draw brim over the sharp edges of the model.</source>
      <translation>仅在模型尖锐边缘启用brim。</translation>
    </message>
    <message>
      <source>Brim ear detection radius</source>
      <translation>圆盘检测半径</translation>
    </message>
    <message>
      <source>The geometry will be decimated before dectecting sharp angles. This parameter indicates the minimum length of the deviation for the decimation.
0 to deactivate</source>
      <translation>在检测尖锐角度之前，几何形状将被简化。此参数表示简化的最小偏差长度。
设为0以停用</translation>
    </message>
    <message>
      <source>Brim ear max angle</source>
      <translation>圆盘最大角度</translation>
    </message>
    <message>
      <source>Maximum angle to let a brim ear appear.
If set to 0, no brim will be created.
If set to ~180, brim will be created on everything but straight sections.</source>
      <translation>让圆盘出现的最大角度。
如果设置为0，则不会创建圆盘。
如果设置为约180，除直线部分外，其他部分都会创建圆盘。</translation>
    </message>
    <message>
      <source>Brim-object gap</source>
      <translation>Brim与模型的间隙</translation>
    </message>
    <message>
      <source>A gap between innermost brim line and object can make brim be removed more easily</source>
      <translation>在brim和模型之间设置间隙，能够让brim更容易剥离</translation>
    </message>
    <message>
      <source>Brim type</source>
      <translation>Brim类型</translation>
    </message>
    <message>
      <source>This controls the generation of the brim at outer and/or inner side of models. Auto means the brim width is analysed and calculated automatically.</source>
      <translation>该参数控制在模型的外侧和/或内侧生成brim。自动是指自动分析和计算边框的宽度。</translation>
    </message>
    <message>
      <source>Auto</source>
      <translation>自动</translation>
    </message>
    <message>
      <source>Mouse ear</source>
      <translation>圆盘</translation>
    </message>
    <message>
      <source>Inner brim only</source>
      <translation>仅内侧</translation>
    </message>
    <message>
      <source>No-brim</source>
      <translation>无brim</translation>
    </message>
    <message>
      <source>Outer and inner brim</source>
      <translation>内侧和外侧</translation>
    </message>
    <message>
      <source>Outer brim only</source>
      <translation>仅外侧</translation>
    </message>
    <message>
      <source>Brim width</source>
      <translation>Brim宽度</translation>
    </message>
    <message>
      <source>Distance from model to the outermost brim line</source>
      <translation>从模型到最外圈brim走线的距离</translation>
    </message>
    <message>
      <source>Chamber temperature</source>
      <translation>机箱温度</translation>
    </message>
    <message>
      <source>Higher chamber temperature can help suppress or reduce warping and potentially lead to higher interlayer bonding strength for high temperature materials like ABS, ASA, PC, PA and so on.At the same time, the air filtration of ABS and ASA will get worse.While for PLA, PETG, TPU, PVA and other low temperature materials,the actual chamber temperature should not be high to avoid cloggings, so 0 which stands for turning off is highly recommended</source>
      <translation>更高的腔温可以帮助抑制或减少翘曲，同时可能会提高高温材料（如ABS、ASA、PC、PA等）的层间粘合强度。与此同时，ABS和ASA的空气过滤性能会变差。而对于PLA、PETG、TPU、PVA等低温材料，为了避免堵塞，实际的腔温不应该过高，因此强烈建议使用0（表示关闭）。</translation>
    </message>
    <message>
      <source>Change extrusion role G-code</source>
      <translation></translation>
    </message>
    <message>
      <source>This gcode is inserted when the extrusion role is changed</source>
      <translation></translation>
    </message>
    <message>
      <source>Change filament G-code</source>
      <translation>耗材丝更换G-code</translation>
    </message>
    <message>
      <source>This gcode is inserted when change filament, including T command to trigger tool change</source>
      <translation>换料时插入的G-code，包括T命令。</translation>
    </message>
    <message>
      <source>No cooling for the first</source>
      <translation>关闭冷却对前</translation>
    </message>
    <message>
      <source>Close all cooling fan for the first certain layers. Cooling fan of the first layer used to be closed to get better build plate adhesion</source>
      <translation>对开始的一些层关闭所有的部件冷却风扇。通常关闭首层冷却用来获得更好的热床粘接</translation>
    </message>
    <message>
      <source>Compatible machine</source>
      <translation>兼容的机器</translation>
    </message>
    <message>
      <source>Compatible machine condition</source>
      <translation>兼容的机器的条件</translation>
    </message>
    <message>
      <source>Compatible process profiles</source>
      <translation>兼容的切片配置</translation>
    </message>
    <message>
      <source>Compatible process profiles condition</source>
      <translation>兼容的切片配置的条件</translation>
    </message>
    <message>
      <source>Speed of exhuast fan after printing completes</source>
      <translation>打印完成后排气风扇的速度</translation>
    </message>
    <message>
      <source>Other layers</source>
      <translation>其它层</translation>
    </message>
    <message>
      <source>Bed temperature for layers except the initial one. Value 0 means the filament does not support to print on the Cool Plate</source>
      <translation>非首层热床温度。0值表示这个耗材丝不支持低温打印热床</translation>
    </message>
    <message>
      <source>Initial layer</source>
      <translation>首层</translation>
    </message>
    <message>
      <source>Bed temperature of the initial layer. Value 0 means the filament does not support to print on the Cool Plate</source>
      <translation>首层热床温度。0值表示这个耗材丝不支持低温打印热床</translation>
    </message>
    <message>
      <source>Cooling tube length</source>
      <translation>喉管长度</translation>
    </message>
    <message>
      <source>Length of the cooling tube to limit space for cooling moves inside it.</source>
      <translation>喉管的长度，用于限制冷却内部移动的空间。</translation>
    </message>
    <message>
      <source>Cooling tube position</source>
      <translation>喉管位置</translation>
    </message>
    <message>
      <source>Distance of the center-point of the cooling tube from the extruder tip.</source>
      <translation>喉管的中心点与挤出机齿尖的距离。</translation>
    </message>
    <message>
      <source>Bed type</source>
      <translation>热床类型</translation>
    </message>
    <message>
      <source>Bed types supported by the printer</source>
      <translation>打印机所支持的热床类型</translation>
    </message>
    <message>
      <source>Cool Plate</source>
      <translation>自定义面板</translation>
    </message>
    <message>
      <source>Engineering Plate</source>
      <translation>工程材料热床</translation>
    </message>
    <message>
      <source>Normal printing</source>
      <translation>普通打印</translation>
    </message>
    <message>
      <source>The default acceleration of both normal printing and travel except initial layer</source>
      <translation>除首层之外的默认的打印和空驶的加速度</translation>
    </message>
    <message>
      <source>Default color</source>
      <translation>缺省颜色</translation>
    </message>
    <message>
      <source>Default filament color</source>
      <translation>缺省材料颜色</translation>
    </message>
    <message>
      <source>Default filament profile</source>
      <translation>默认耗材配置</translation>
    </message>
    <message>
      <source>Default filament profile when switch to this machine profile</source>
      <translation>该机器配置的默认耗材配置</translation>
    </message>
    <message>
      <source>Default</source>
      <translation>缺省</translation>
    </message>
    <message>
      <source>Default process profile</source>
      <translation>默认切片配置</translation>
    </message>
    <message>
      <source>Default process profile when switch to this machine profile</source>
      <translation>该机器的默认切片配置</translation>
    </message>
    <message>
      <source>Deretraction Speed</source>
      <translation>装填速度</translation>
    </message>
    <message>
      <source>Speed for reloading filament into extruder. Zero means same speed with retraction</source>
      <translation>耗材丝装填的速度，0表示和回抽速度一致。</translation>
    </message>
    <message>
      <source>Detect narrow internal solid infill</source>
      <translation>识别狭窄内部实心填充</translation>
    </message>
    <message>
      <source>This option will auto detect narrow internal solid infill area. If enabled, concentric pattern will be used for the area to speed printing up. Otherwise, rectilinear pattern is used defaultly.</source>
      <translation>此选项用于自动识别内部狭窄的实心填充。开启后，将对狭窄实心区域使用同心填充加快打印速度。否则使用默认的直线填充。</translation>
    </message>
    <message>
      <source>Detect overhang wall</source>
      <translation>识别悬空外墙</translation>
    </message>
    <message>
      <source>Detect the overhang percentage relative to line width and use different speed to print. For 100%% overhang, bridge speed is used.</source>
      <translation>检测悬空相对于线宽的百分比，并应用不同的速度打印。100%%的悬空将使用桥接速度。</translation>
    </message>
    <message>
      <source>Detect thin wall</source>
      <translation>检查薄壁</translation>
    </message>
    <message>
      <source>Detect thin wall which can't contain two line width. And use single line to print. Maybe printed not very well, because it's not closed loop</source>
      <translation>检查无法容纳两条走线的薄壁。使用单条走线打印。可能会打地不是很好，因为走线不再闭合。</translation>
    </message>
    <message>
      <source>Disabled</source>
      <translation></translation>
    </message>
    <message>
      <source>Speed of exhuast fan during printing.This speed will overwrite the speed in filament custom gcode</source>
      <translation>打印过程中排气风扇的速度。此速度将覆盖耗材丝自定义gcode中的速度</translation>
    </message>
    <message>
      <source>Elephant foot compensation</source>
      <translation>象脚补偿</translation>
    </message>
    <message>
      <source>Shrink the initial layer on build plate to compensate for elephant foot effect</source>
      <translation>将首层收缩用于补偿象脚效应</translation>
    </message>
    <message>
      <source>Elephant foot compensation layers</source>
      <translation>象脚补偿层数</translation>
    </message>
    <message>
      <source>The number of layers on which the elephant foot compensation will be active. The first layer will be shrunk by the elephant foot compensation value, then the next layers will be linearly shrunk less, up to the layer indicated by this value.</source>
      <translation></translation>
    </message>
    <message>
      <source>Arc fitting</source>
      <translation>圆弧拟合</translation>
    </message>
    <message>
      <source>Enable this to get a G-code file which has G2 and G3 moves. And the fitting tolerance is same with resolution</source>
      <translation>打开这个设置，导出的G-code将包含G2 G3指令。圆弧拟合的容许值和精度相同。</translation>
    </message>
    <message>
      <source>Enable filament ramming</source>
      <translation>启用耗材尖端成型</translation>
    </message>
    <message>
      <source>Force cooling for overhang and bridge</source>
      <translation>悬垂/桥接强制冷却</translation>
    </message>
    <message>
      <source>Enable this option to optimize part cooling fan speed for overhang and bridge to get better cooling</source>
      <translation>勾选这个选项将自动优化桥接和悬垂的风扇转速以获得更好的冷却</translation>
    </message>
    <message>
      <source>Slow down for overhang</source>
      <translation>悬垂降速</translation>
    </message>
    <message>
      <source>Enable this option to slow printing down for different overhang degree</source>
      <translation>打开这个选项将降低不同悬垂程度的走线的打印速度</translation>
    </message>
    <message>
      <source>Enable pressure advance</source>
      <translation>启用压力提前</translation>
    </message>
    <message>
      <source>Enable pressure advance, auto calibration result will be overwriten once enabled.</source>
      <translation>启用压力提前，一旦启用会覆盖自动检测的结果</translation>
    </message>
    <message>
      <source>Enable</source>
      <translation>开启</translation>
    </message>
    <message>
      <source>The wiping tower can be used to clean up the residue on the nozzle and stabilize the chamber pressure inside the nozzle, in order to avoid appearance defects when printing objects.</source>
      <translation>擦拭塔可以用来清理喷嘴上的残留料和让喷嘴内部的腔压达到稳定状态，以避免打印物体时出现外观瑕疵。</translation>
    </message>
    <message>
      <source>Enable support</source>
      <translation>开启支撑</translation>
    </message>
    <message>
      <source>Enable support generation.</source>
      <translation>开启支撑生成。</translation>
    </message>
    <message>
      <source>Bed temperature for layers except the initial one. Value 0 means the filament does not support to print on the Engineering Plate</source>
      <translation>非首层热床温度。0值表示这个耗材丝不支持工程材料热床</translation>
    </message>
    <message>
      <source>Bed temperature of the initial layer. Value 0 means the filament does not support to print on the Engineering Plate</source>
      <translation>首层热床温度。0值表示这个耗材丝不支持工程材料热床</translation>
    </message>
    <message>
      <source>Ensure vertical shell thickness</source>
      <translation>确保垂直外壳厚度</translation>
    </message>
    <message>
      <source>Add solid infill near sloping surfaces to guarantee the vertical shell thickness (top+bottom solid layers)</source>
      <translation>在斜面表面附近添加实心填充，以保证垂直外壳厚度（顶部+底部实心层）</translation>
    </message>
    <message>
      <source>Exclude objects</source>
      <translation>对象排除</translation>
    </message>
    <message>
      <source>Enable this option to add EXCLUDE OBJECT command in g-code</source>
      <translation>开启此选项以支持对象排除</translation>
    </message>
    <message>
      <source>Extra loading distance</source>
      <translation>额外加载距离</translation>
    </message>
    <message>
      <source>When set to zero, the distance the filament is moved from parking position during load is exactly the same as it was moved back during unload. When positive, it is loaded further,  if negative, the loading move is shorter than unloading.</source>
      <translation>当设置为零时，耗材的加载移动与卸载移动的距离相同。如果为正，加载比卸载长。如果为负，加载比卸载短。</translation>
    </message>
    <message>
      <source>Extra perimeters on overhangs</source>
      <translation>悬垂上的额外周长</translation>
    </message>
    <message>
      <source>Create additional perimeter paths over steep overhangs and areas where bridges cannot be anchored. </source>
      <translation>在陡峭的悬垂和无法固定桥接的区域上创建额外的周长路径。</translation>
    </message>
    <message>
      <source>Height to lid</source>
      <translation>到顶盖高度</translation>
    </message>
    <message>
      <source>Distance of the nozzle tip to the lid. Used for collision avoidance in by-object printing.</source>
      <translation>喷嘴尖端到顶盖的距离。用于在逐件打印中避免碰撞。</translation>
    </message>
    <message>
      <source>Height to rod</source>
      <translation>到横杆高度</translation>
    </message>
    <message>
      <source>Distance of the nozzle tip to the lower rod. Used for collision avoidance in by-object printing.</source>
      <translation>喷嘴尖端到下方滑杆的距离。用于在逐件打印中避免碰撞。</translation>
    </message>
    <message>
      <source>Radius</source>
      <translation>半径</translation>
    </message>
    <message>
      <source>Clearance radius around extruder. Used for collision avoidance in by-object printing.</source>
      <translation>挤出机四周的避让半径。用于在逐件打印中避免碰撞。</translation>
    </message>
    <message>
      <source>Extruder Color</source>
      <translation>挤出机颜色</translation>
    </message>
    <message>
      <source>Only used as a visual help on UI</source>
      <translation>只作为界面上的可视化辅助</translation>
    </message>
    <message>
      <source>Extruder offset</source>
      <translation>挤出机偏移</translation>
    </message>
    <message>
      <source>Layer time</source>
      <translation>层时间</translation>
    </message>
    <message>
      <source>Part cooling fan will be enabled for layers of which estimated time is shorter than this value. Fan speed is interpolated between the minimum and maximum fan speeds according to layer printing time</source>
      <translation>当层预估打印时间小于该数值时，部件冷却风扇将会被开启。风扇转速将根据层打印时间在最大和最小风扇转速之间插值获得</translation>
    </message>
    <message>
      <source>Fan kick-start time</source>
      <translation>风扇</translation>
    </message>
    <message>
      <source>Emit a max fan speed command for this amount of seconds before reducing to target speed to kick-start the cooling fan.
This is useful for fans where a low PWM/power may be insufficient to get the fan started spinning from a stop, or to get the fan up to speed faster.
Set to 0 to deactivate.</source>
      <translation>让风扇满速运行指定时间以帮助风扇顺利启动
设为0禁用此选项</translation>
    </message>
    <message>
      <source>Part cooling fan speed may be increased when auto cooling is enabled. This is the maximum speed limitation of part cooling fan</source>
      <translation>启用自动冷却时，可能会提高部件冷却风扇的转速。这是部件冷却风扇的最大速度限制</translation>
    </message>
    <message>
      <source>Minimum speed for part cooling fan</source>
      <translation>部件冷却风扇的最小转速</translation>
    </message>
    <message>
      <source>Only overhangs</source>
      <translation>仅悬垂</translation>
    </message>
    <message>
      <source>Will only take into account the delay for the cooling of overhangs.</source>
      <translation>仅对悬垂有效</translation>
    </message>
    <message>
      <source>Fan speed-up time</source>
      <translation>风扇响应时间</translation>
    </message>
    <message>
      <source>Start the fan this number of seconds earlier than its target start time (you can use fractional seconds). It assumes infinite acceleration for this time estimation, and will only take into account G1 and G0 moves (arc fitting is unsupported).
It won't move fan comands from custom gcodes (they act as a sort of 'barrier').
It won't move fan comands into the start gcode if the 'only custom start gcode' is activated.
Use 0 to deactivate.</source>
      <translation>把风扇启动指令往前移动指定时间以补偿风扇的启动时间。目前支支持G1 G0指令
设为0以禁用此选项</translation>
    </message>
    <message>
      <source>Color</source>
      <translation>颜色</translation>
    </message>
    <message>
      <source>Speed of the last cooling move</source>
      <translation>最后一次冷却移动的速度</translation>
    </message>
    <message>
      <source>Cooling moves are gradually accelerating towards this speed.</source>
      <translation>冷却移动向这个速度逐渐加速。</translation>
    </message>
    <message>
      <source>Speed of the first cooling move</source>
      <translation>第一次冷却移动的速度</translation>
    </message>
    <message>
      <source>Cooling moves are gradually accelerating beginning at this speed.</source>
      <translation>从这个速度开始冷却移动逐渐加速</translation>
    </message>
    <message>
      <source>Number of cooling moves</source>
      <translation>冷却移动次数</translation>
    </message>
    <message>
      <source>Filament is cooled by being moved back and forth in the cooling tubes. Specify desired number of these moves.</source>
      <translation>耗材丝通过在喉管中来回移动来冷却。指定所需的移动次数。</translation>
    </message>
    <message>
      <source>Price</source>
      <translation>价格</translation>
    </message>
    <message>
      <source>Filament price. For statistics only</source>
      <translation>耗材丝的价格。只用于统计信息。</translation>
    </message>
    <message>
      <source>Density</source>
      <translation>密度</translation>
    </message>
    <message>
      <source>Filament density. For statistics only</source>
      <translation>耗材丝的密度。只用于统计信息。</translation>
    </message>
    <message>
      <source>Diameter</source>
      <translation>直径</translation>
    </message>
    <message>
      <source>Filament diameter is used to calculate extrusion in gcode, so it's important and should be accurate</source>
      <translation>耗材丝直径被用于计算G-code文件中的挤出量。因此很重要，应尽可能精确。</translation>
    </message>
    <message>
      <source>End G-code</source>
      <translation>结尾G-code</translation>
    </message>
    <message>
      <source>End G-code when finish the printing of this filament</source>
      <translation>结束使用该耗材打印时的结尾G-code</translation>
    </message>
    <message>
      <source>Flow ratio</source>
      <translation>流量比例</translation>
    </message>
    <message>
      <source>The material may have volumetric change after switching between molten state and crystalline state. This setting changes all extrusion flow of this filament in gcode proportionally. Recommended value range is between 0.95 and 1.05. Maybe you can tune this value to get nice flat surface when there has slight overflow or underflow</source>
      <translation>材料经过融化后凝固可能会产生体积差异。这个设置会等比例改变所有挤出走线的挤出量。推荐的范围为0.95到1.05。发现大平层模型的顶面有轻微的缺料或多料时，或许可以尝试微调这个参数。</translation>
    </message>
    <message>
      <source>Support material</source>
      <translation>支撑材料</translation>
    </message>
    <message>
      <source>Support material is commonly used to print support and support interface</source>
      <translation>支撑材料通常用于打印支撑体和支撑接触面</translation>
    </message>
    <message>
      <source>Filament load time</source>
      <translation>加载耗材丝的时间</translation>
    </message>
    <message>
      <source>Time for the printer firmware (or the Multi Material Unit 2.0) to load a new filament during a tool change (when executing the T code). This time is added to the total print time by the G-code time estimator.</source>
      <translation>在换色时（执行T代码，如T1，T2），打印机固件（或Multi Material Unit 2.0）加载新耗材的所需时间。该时间将会被G-code时间评估功能加到总打印时间上去。</translation>
    </message>
    <message>
      <source>Loading speed</source>
      <translation>加载速度</translation>
    </message>
    <message>
      <source>Speed used for loading the filament on the wipe tower.</source>
      <translation>用于擦拭塔加载耗材的速度。</translation>
    </message>
    <message>
      <source>Loading speed at the start</source>
      <translation>加载初始速度</translation>
    </message>
    <message>
      <source>Speed used at the very beginning of loading phase.</source>
      <translation>最开始加载阶段的速度</translation>
    </message>
    <message>
      <source>Max volumetric speed</source>
      <translation>最大体积速度</translation>
    </message>
    <message>
      <source>This setting stands for how much volume of filament can be melted and extruded per second. Printing speed is limited by max volumetric speed, in case of too high and unreasonable speed setting. Can't be zero</source>
      <translation>这个设置表示在1秒内能够融化和挤出的耗材丝体积。打印速度会受到最大体积速度的限制，防止设置过高和不合理的速度。不允许设置为0。</translation>
    </message>
    <message>
      <source>Minimal purge on wipe tower</source>
      <translation>擦拭塔上的最小清理量</translation>
    </message>
    <message>
      <source>Enable ramming for multitool setups</source>
      <translation>启用多色尖端成型设置</translation>
    </message>
    <message>
      <source>Perform ramming when using multitool printer (i.e. when the 'Single Extruder Multimaterial' in Printer Settings is unchecked). When checked, a small amount of filament is rapidly extruded on the wipe tower just before the toolchange. This option is only used when the wipe tower is enabled.</source>
      <translation>多色打印机执行尖端成型时（即，当打印机设置中的单挤出机多材料未选中时）。选中时，在换色之前，会迅速挤出少量耗材丝到擦拭塔上。此选项仅在启用擦拭塔时使用。</translation>
    </message>
    <message>
      <source>Multitool ramming flow</source>
      <translation>多色尖端成型流量</translation>
    </message>
    <message>
      <source>Flow used for ramming the filament before the toolchange.</source>
      <translation>换色前耗材尖端成型的流量</translation>
    </message>
    <message>
      <source>Multitool ramming volume</source>
      <translation>多色尖端成型体积</translation>
    </message>
    <message>
      <source>The volume to be rammed before the toolchange.</source>
      <translation>换色前尖端成型的体积</translation>
    </message>
    <message>
      <source>Filament notes</source>
      <translation>耗材注释</translation>
    </message>
    <message>
      <source>You can put your notes regarding the filament here.</source>
      <translation>你可以把关于耗材的注释放在这里。</translation>
    </message>
    <message>
      <source>Ramming parameters</source>
      <translation>尖端成型参数</translation>
    </message>
    <message>
      <source>This string is edited by RammingDialog and contains ramming specific parameters.</source>
      <translation>此内容由尖端成型窗口编辑，包含尖端成型的特定参数。</translation>
    </message>
    <message>
      <source>Retract amount before wipe</source>
      <translation>擦拭前的回抽量</translation>
    </message>
    <message>
      <source>The length of fast retraction before wipe, relative to retraction length</source>
      <translation>擦拭之前的回抽长度，用总回抽长度的百分比表示。</translation>
    </message>
    <message>
      <source>Only lift Z above</source>
      <translation>仅在高度以上抬Z</translation>
    </message>
    <message>
      <source>If you set this to a positive value, Z lift will only take place above the specified absolute Z.</source>
      <translation>如果将其设置为正值，则Z抬升仅在指定的绝对Z之上发生</translation>
    </message>
    <message>
      <source>Only lift Z below</source>
      <translation>仅在高度以下抬Z</translation>
    </message>
    <message>
      <source>If you set this to a positive value, Z lift will only take place below the specified absolute Z.</source>
      <translation>如果将其设置为正值，则Z抬升仅在指定的绝对Z以下发生。</translation>
    </message>
    <message>
      <source>On surfaces</source>
      <translation>仅表面抬Z</translation>
    </message>
    <message>
      <source>Enforce Z Hop behavior. This setting is impacted by the above settings (Only lift Z above/below).</source>
      <translation>强制Z抬升行为。此设置受上述设置的影响（仅在高度以下抬Z/仅在高度以上抬Z）。</translation>
    </message>
    <message>
      <source>All Surfaces</source>
      <translation>所有表面</translation>
    </message>
    <message>
      <source>Bottom Only</source>
      <translation>仅底面</translation>
    </message>
    <message>
      <source>Top Only</source>
      <translation>仅顶面</translation>
    </message>
    <message>
      <source>Top and Bottom</source>
      <translation>顶面和地面</translation>
    </message>
    <message>
      <source>Extra length on restart</source>
      <translation>额外回填长度</translation>
    </message>
    <message>
      <source>When the retraction is compensated after the travel move, the extruder will push this additional amount of filament. This setting is rarely needed.</source>
      <translation>每当空驶后回抽被补偿时，挤出机将推入额外数量的耗材丝。很少需要此设置。</translation>
    </message>
    <message>
      <source>Retract when change layer</source>
      <translation>换层时回抽</translation>
    </message>
    <message>
      <source>Force a retraction when changes layer</source>
      <translation>强制在换层时回抽。</translation>
    </message>
    <message>
      <source>Length</source>
      <translation>长度</translation>
    </message>
    <message>
      <source>Some amount of material in extruder is pulled back to avoid ooze during long travel. Set zero to disable retraction</source>
      <translation>挤出机中的一些材料会被拉回特定长度，避免空驶较长时材料渗出。设置为0表示关闭回抽。</translation>
    </message>
    <message>
      <source>Travel distance threshold</source>
      <translation>空驶距离阈值</translation>
    </message>
    <message>
      <source>Only trigger retraction when the travel distance is longer than this threshold</source>
      <translation>只在空驶距离大于该数值时触发回抽。</translation>
    </message>
    <message>
      <source>Retraction Speed</source>
      <translation>回抽速度</translation>
    </message>
    <message>
      <source>Speed of retractions</source>
      <translation>回抽速度</translation>
    </message>
    <message>
      <source>Shrinkage</source>
      <translation>耗材收缩率</translation>
    </message>
    <message>
      <source>Enter the shrinkage percentage that the filament will get after cooling (94% if you measure 94mm instead of 100mm). The part will be scaled in xy to compensate. Only the filament used for the perimeter is taken into account.
Be sure to allow enough space between objects, as this compensation is done after the checks.</source>
      <translation>冷却后耗材会收缩的百分比(如果测量的长度是94mm而不是100mm，则为是收缩率为94%)
补偿将按比例缩放xy轴该补偿仅考虑墙壁所使用的耗材
请确保物体之间有足够的间距，因为补偿是在边界检查之后进行</translation>
    </message>
    <message>
      <source>Soluble material</source>
      <translation>可溶性材料</translation>
    </message>
    <message>
      <source>Soluble material is commonly used to print support and support interface</source>
      <translation>可溶性材料通常用于打印支撑和支撑面</translation>
    </message>
    <message>
      <source>Start G-code</source>
      <translation>起始G-code</translation>
    </message>
    <message>
      <source>Start G-code when start the printing of this filament</source>
      <translation>开始使用这个耗材丝打印的起始G-code</translation>
    </message>
    <message>
      <source>Delay after unloading</source>
      <translation>卸载后延迟</translation>
    </message>
    <message>
      <source>Time to wait after the filament is unloaded. May help to get reliable toolchanges with flexible materials that may need more time to shrink to original dimensions.</source>
      <translation>耗材丝卸载后等待的时间。有助于柔性材料（收缩到原始尺寸需更多的时间）以获得可靠的换色。</translation>
    </message>
    <message>
      <source>Type</source>
      <translation>类型</translation>
    </message>
    <message>
      <source>The material type of filament</source>
      <translation>耗材丝的材料类型</translation>
    </message>
    <message>
      <source>Filament unload time</source>
      <translation>卸载耗材丝的时间</translation>
    </message>
    <message>
      <source>Time for the printer firmware (or the Multi Material Unit 2.0) to unload a filament during a tool change (when executing the T code). This time is added to the total print time by the G-code time estimator.</source>
      <translation>换色期间（执行T代码时如T1，T2），打印机固件（或MMU2.0）卸载耗材所需时间。该时间将会被G-code时间评估功能加到总打印时间上去。</translation>
    </message>
    <message>
      <source>Unloading speed</source>
      <translation>卸载速度</translation>
    </message>
    <message>
      <source>Speed used for unloading the filament on the wipe tower (does not affect  initial part of unloading just after ramming).</source>
      <translation>用于擦拭塔卸载耗材的速度（不影响尖端成型之后初始部分的速度）。</translation>
    </message>
    <message>
      <source>Unloading speed at the start</source>
      <translation>卸载初始速度</translation>
    </message>
    <message>
      <source>Speed used for unloading the tip of the filament immediately after ramming.</source>
      <translation>耗材尖端成型后立即卸载的速度</translation>
    </message>
    <message>
      <source>Vendor</source>
      <translation>供应商</translation>
    </message>
    <message>
      <source>Vendor of filament. For show only</source>
      <translation>打印耗材的供应商。仅用于展示。</translation>
    </message>
    <message>
      <source>Wipe while retracting</source>
      <translation>回抽时擦拭</translation>
    </message>
    <message>
      <source>Move nozzle along the last extrusion path when retracting to clean leaked material on nozzle. This can minimize blob when print new part after travel</source>
      <translation>当回抽时，让喷嘴沿着前面的走线方向继续移动，清除掉喷嘴上的漏料。这能够避免空驶结束打印新的区域时产生斑点。</translation>
    </message>
    <message>
      <source>Wipe Distance</source>
      <translation>擦拭距离</translation>
    </message>
    <message>
      <source>Discribe how long the nozzle will move along the last path when retracting</source>
      <translation>表示回抽时擦拭的移动距离。</translation>
    </message>
    <message>
      <source>Z hop when retract</source>
      <translation>回抽时抬升Z</translation>
    </message>
    <message>
      <source>Whenever the retraction is done, the nozzle is lifted a little to create clearance between nozzle and the print. It prevents nozzle from hitting the print when travel move. Using spiral line to lift z can prevent stringing</source>
      <translation>回抽完成之后，喷嘴轻微抬升，和打印件之间产生一定间隙。这能够避免空驶时喷嘴和打印件剐蹭和碰撞。使用螺旋线抬升z能够减少拉丝。</translation>
    </message>
    <message>
      <source>Z hop type</source>
      <translation>抬Z类型</translation>
    </message>
    <message>
      <source>Normal</source>
      <translation>普通</translation>
    </message>
    <message>
      <source>Slope</source>
      <translation>梯形</translation>
    </message>
    <message>
      <source>Spiral</source>
      <translation>螺旋</translation>
    </message>
    <message>
      <source>Filename format</source>
      <translation>文件名格式</translation>
    </message>
    <message>
      <source>User can self-define the project file name when export</source>
      <translation>用户可以自定义导出项目文件的名称。</translation>
    </message>
    <message>
      <source>Filter out tiny gaps</source>
      <translation>忽略微小间隙</translation>
    </message>
    <message>
      <source>Filter out gaps smaller than the threshold specified</source>
      <translation>忽略小于指定阈值的间隙</translation>
    </message>
    <message>
      <source>First layer print sequence</source>
      <translation>第一层打印顺序</translation>
    </message>
    <message>
      <source>Flush into objects' infill</source>
      <translation>冲刷到对象的填充</translation>
    </message>
    <message>
      <source>Purging after filament change will be done inside objects' infills. This may lower the amount of waste and decrease the print time. If the walls are printed with transparent filament, the mixed color infill will be seen outside. It will not take effect, unless the prime tower is enabled.</source>
      <translation>换料后的过渡料会被用来打印对象的填充。这样可以减少材料浪费和缩短打印时间，但是如果对象的内外墙是采用透明材料打印的，则可以从模型外观上看到内部的混色过渡料。该功能只有在启用料塔的时候才生效。</translation>
    </message>
    <message>
      <source>Flush into this object</source>
      <translation>冲刷到这个对象</translation>
    </message>
    <message>
      <source>This object will be used to purge the nozzle after a filament change to save filament and decrease the print time. Colours of the objects will be mixed as a result. It will not take effect, unless the prime tower is enabled.</source>
      <translation>换料后的过渡料会被用来打印这个对象。这样可以减少材料浪费和缩短打印时间，但是这个对象的外观会是混色的。该功能只有在启用料塔的时候才生效。</translation>
    </message>
    <message>
      <source>Flush into objects' support</source>
      <translation>冲刷到对象的支撑</translation>
    </message>
    <message>
      <source>Purging after filament change will be done inside objects' support. This may lower the amount of waste and decrease the print time. It will not take effect, unless the prime tower is enabled.</source>
      <translation>换料后的过渡料会被用来打印对象的支撑。这样可以减少材料浪费以及缩短打印时间。该功能只有在启用料塔的时候才生效。</translation>
    </message>
    <message>
      <source>Flush multiplier</source>
      <translation>冲刷量乘数</translation>
    </message>
    <message>
      <source>The actual flushing volumes is equal to the flush multiplier multiplied by the flushing volumes in the table.</source>
      <translation>实际冲刷量等于冲刷量乘数乘以表格单元中的冲刷量</translation>
    </message>
    <message>
      <source>Purging volumes</source>
      <translation>冲刷体积</translation>
    </message>
    <message>
      <source>Purging volumes - load/unload volumes</source>
      <translation>清理量 - 加载/卸载量</translation>
    </message>
    <message>
      <source>Full fan speed at layer</source>
      <translation>满速风扇在</translation>
    </message>
    <message>
      <source>Fan speed will be ramped up linearly from zero at layer &quot;close_fan_the_first_x_layers&quot; to maximum at layer &quot;full_fan_speed_layer&quot;. &quot;full_fan_speed_layer&quot; will be ignored if lower than &quot;close_fan_the_first_x_layers&quot;, in which case the fan will be running at maximum allowed speed at layer &quot;close_fan_the_first_x_layers&quot; + 1.</source>
      <translation>风扇速度将从“禁用第一层”的零线性上升到“全风扇速度层”的最大。如果低于“禁用风扇第一层”，则“全风扇速度第一层”将被忽略，在这种情况下，风扇将在“禁用风扇第一层”+1层以最大允许速度运行。</translation>
    </message>
    <message>
      <source>Fuzzy Skin</source>
      <translation>绒毛表面</translation>
    </message>
    <message>
      <source>Randomly jitter while printing the wall, so that the surface has a rough look. This setting controls the fuzzy position</source>
      <translation>打印外墙时随机抖动，使外表面产生绒效果。这个设置决定适用的位置。</translation>
    </message>
    <message>
      <source>Contour and hole</source>
      <translation>轮廓和孔</translation>
    </message>
    <message>
      <source>All walls</source>
      <translation>所有墙</translation>
    </message>
    <message>
      <source>Contour</source>
      <translation>轮廓</translation>
    </message>
    <message>
      <source>None</source>
      <translation>无</translation>
    </message>
    <message>
      <source>Fuzzy skin point distance</source>
      <translation>绒毛表面点间距</translation>
    </message>
    <message>
      <source>The average diatance between the random points introducded on each line segment</source>
      <translation>产生绒毛表面时，插入的随机点之间的平均距离</translation>
    </message>
    <message>
      <source>Fuzzy skin thickness</source>
      <translation>绒毛表面厚度</translation>
    </message>
    <message>
      <source>The width within which to jitter. It's adversed to be below outer wall line width</source>
      <translation>产生绒毛的抖动的宽度。建议小于外圈墙的线宽。</translation>
    </message>
    <message>
      <source>Gap infill</source>
      <translation>填缝</translation>
    </message>
    <message>
      <source>Speed of gap infill. Gap usually has irregular line width and should be printed more slowly</source>
      <translation>填缝的速度。缝隙通常有不一致的线宽，应改用较慢速度打印。</translation>
    </message>
    <message>
      <source>Add line number</source>
      <translation>标注行号</translation>
    </message>
    <message>
      <source>Enable this to add line number(Nx) at the beginning of each G-Code line</source>
      <translation>打开这个设置，G-code的每一行的开头会增加Nx标注行号。</translation>
    </message>
    <message>
      <source>Verbose G-code</source>
      <translation>注释G-code</translation>
    </message>
    <message>
      <source>Enable this to get a commented G-code file, with each line explained by a descriptive text. If you print from SD card, the additional weight of the file could make your firmware slow down.</source>
      <translation></translation>
    </message>
    <message>
      <source>G-code flavor</source>
      <translation>G-code风格</translation>
    </message>
    <message>
      <source>What kind of gcode the printer is compatible with</source>
      <translation>打印机兼容的G-code风格'</translation>
    </message>
    <message>
      <source>Klipper</source>
      <translation></translation>
    </message>
    <message>
      <source>Label objects</source>
      <translation>标注模型</translation>
    </message>
    <message>
      <source>Enable this to add comments into the G-Code labeling print moves with what object they belong to, which is useful for the Octoprint CancelObject plugin. This settings is NOT compatible with Single Extruder Multi Material setup and Wipe into Object / Wipe into Infill.</source>
      <translation></translation>
    </message>
    <message>
      <source>High extruder current on filament swap</source>
      <translation>更换耗材挤出机大电流</translation>
    </message>
    <message>
      <source>It may be beneficial to increase the extruder motor current during the filament exchange sequence to allow for rapid ramming feed rates and to overcome resistance when loading a filament with an ugly shaped tip.</source>
      <translation>可能有益于换耗材过程中增加挤出机电流，克服加载耗材时的阻力以加快尖端成型进料速率从而避免产生难看形状的尖端。</translation>
    </message>
    <message>
      <source>Convert holes to polyholes</source>
      <translation>将圆孔转换为多边型孔</translation>
    </message>
    <message>
      <source>Search for almost-circular holes that span more than one layer and convert the geometry to polyholes. Use the nozzle size and the (biggest) diameter to compute the polyhole.
See http://hydraraptor.blogspot.com/2011/02/polyholes.html</source>
      <translation>搜索跨越多层的近似圆形孔，并将几何形状转换为多边形孔。使用喷嘴尺寸和（最大）直径来计算多边形孔。
参见http://hydraraptor.blogspot.com/2011/02/polyholes.html</translation>
    </message>
    <message>
      <source>Polyhole detection margin</source>
      <translation>多边型孔检测边缘</translation>
    </message>
    <message>
      <source>Maximum defection of a point to the estimated radius of the circle.
As cylinders are often exported as triangles of varying size, points may not be on the circle circumference. This setting allows you some leway to broaden the detection.
In mm or in % of the radius.</source>
      <translation>点到圆半径的最大偏差。
由于圆柱体通常被导出为大小不同的三角形，因此点可能不在圆周上。
此设置允许您有一些余地来扩大检测范围。
以毫米或半径的百分比表示。</translation>
    </message>
    <message>
      <source>Polyhole twist</source>
      <translation>扭曲多边型孔</translation>
    </message>
    <message>
      <source>Rotate the polyhole every layer.</source>
      <translation>按层旋转多边形孔。</translation>
    </message>
    <message>
      <source>Host Type</source>
      <translation>主机类型</translation>
    </message>
    <message>
      <source>Bed temperature for layers except the initial one. Value 0 means the filament does not support to print on the High Temp Plate</source>
      <translation>非首层热床温度。0值表示这个耗材丝不支持高温打印热床</translation>
    </message>
    <message>
      <source>Bed temperature of the initial layer. Value 0 means the filament does not support to print on the High Temp Plate</source>
      <translation>首层热床温度。0值表示这个耗材丝不支持高温打印热床</translation>
    </message>
    <message>
      <source>Independent support layer height</source>
      <translation>支撑独立层高</translation>
    </message>
    <message>
      <source>Support layer uses layer height independent with object layer. This is to support customizing z-gap and save print time.This option will be invalid when the prime tower is enabled.</source>
      <translation>支撑层使用与对象层独立的层高。这是为了支持自定义z-gap并且节省打印时间。当擦料塔被启用时，这个选项将无效。</translation>
    </message>
    <message>
      <source>Sparse infill anchor length</source>
      <translation>稀疏填充锚线长度</translation>
    </message>
    <message>
      <source>Maximum length of the infill anchor</source>
      <translation>填充锚线的最大长度</translation>
    </message>
    <message>
      <source>Infill combination</source>
      <translation>合并填充</translation>
    </message>
    <message>
      <source>Automatically Combine sparse infill of several layers to print together to reduce time. Wall is still printed with original layer height.</source>
      <translation>自动合并若干层稀疏填充一起打印以可缩短时间。内外墙依然保持原始层高打印。</translation>
    </message>
    <message>
      <source>Infill direction</source>
      <translation>填充方向</translation>
    </message>
    <message>
      <source>Angle for sparse infill pattern, which controls the start or main direction of line</source>
      <translation>稀疏填充图案的角度，决定走线的开始或整体方向。</translation>
    </message>
    <message>
      <source>Jerk for infill</source>
      <translation>填充抖动</translation>
    </message>
    <message>
      <source>Infill/Wall overlap</source>
      <translation>填充/墙 重叠</translation>
    </message>
    <message>
      <source>Infill area is enlarged slightly to overlap with wall for better bonding. The percentage value is relative to line width of sparse infill</source>
      <translation>填充区域被轻微扩大，并和外墙产生重叠，进而产生更好的粘接。表示为相对稀疏填充的线宽的百分比。</translation>
    </message>
    <message>
      <source>Acceleration of initial layer. Using a lower value can improve build plate adhesive</source>
      <translation>首层加速度。使用较低值可以改善和构建板的粘接。</translation>
    </message>
    <message>
      <source>Initial layer infill</source>
      <translation>首层填充</translation>
    </message>
    <message>
      <source>Speed of solid infill part of initial layer</source>
      <translation>首层实心填充的打印速度</translation>
    </message>
    <message>
      <source>Jerk for initial layer</source>
      <translation>首层抖动值</translation>
    </message>
    <message>
      <source>Line width of initial layer. If expressed as a %, it will be computed over the nozzle diameter.</source>
      <translation>首层的线宽。如果以%表示，它将基于喷嘴直径来计算。</translation>
    </message>
    <message>
      <source>First layer minimum wall width</source>
      <translation>首层墙最小线宽</translation>
    </message>
    <message>
      <source>The minimum wall width that should be used for the first layer is recommended to be set to the same size as the nozzle. This adjustment is expected to enhance adhesion.</source>
      <translation>应用于首层的墙最小线宽，建议设置与喷嘴尺寸相同。这种调整有助于增强附着力。</translation>
    </message>
    <message>
      <source>Initial layer height</source>
      <translation>首层层高</translation>
    </message>
    <message>
      <source>Height of initial layer. Making initial layer height to be thick slightly can improve build plate adhension</source>
      <translation>首层层高</translation>
    </message>
    <message>
      <source>Speed of initial layer except the solid infill part</source>
      <translation>首层除实心填充之外的其他部分的打印速度</translation>
    </message>
    <message>
      <source>Initial layer travel speed</source>
      <translation>首层空驶速度</translation>
    </message>
    <message>
      <source>Travel speed of initial layer</source>
      <translation>首层空驶速度</translation>
    </message>
    <message>
      <source>Inner wall</source>
      <translation>内墙</translation>
    </message>
    <message>
      <source>Acceleration of inner walls</source>
      <translation>内圈墙加速度，使用较低值可以改善质量。</translation>
    </message>
    <message>
      <source>Jerk of inner walls</source>
      <translation>内墙抖动值</translation>
    </message>
    <message>
      <source>Line width of inner wall. If expressed as a %, it will be computed over the nozzle diameter.</source>
      <translation>内墙的线宽。如果以%表示，它将基于喷嘴直径来计算。</translation>
    </message>
    <message>
      <source>Speed of inner wall</source>
      <translation>内圈墙打印速度</translation>
    </message>
    <message>
      <source>Interface shells</source>
      <translation>接触面外壳</translation>
    </message>
    <message>
      <source>Force the generation of solid shells between adjacent materials/volumes. Useful for multi-extruder prints with translucent materials or manual soluble support material</source>
      <translation>强制在相邻材料/体积之间生成实心壳。适用于使用半透明材料或手动可溶性支撑材料的多挤出机打印</translation>
    </message>
    <message>
      <source>Internal bridge flow ratio</source>
      <translation>内部搭桥流量比例</translation>
    </message>
    <message>
      <source>This value governs the thickness of the internal bridge layer. This is the first layer over sparse infill. Decrease this value slightly (for example 0.9) to improve surface quality over sparse infill.</source>
      <translation></translation>
    </message>
    <message>
      <source>Internal</source>
      <translation>内部</translation>
    </message>
    <message>
      <source>Speed of internal bridge. If the value is expressed as a percentage, it will be calculated based on the bridge_speed. Default value is 150%.</source>
      <translation>内部桥接的速度。如果该值以百分比表示，则将根据桥接速度进行计算。默认值为150%。</translation>
    </message>
    <message>
      <source>Internal solid infill</source>
      <translation>内部实心填充</translation>
    </message>
    <message>
      <source>Acceleration of internal solid infill. If the value is expressed as a percentage (e.g. 100%), it will be calculated based on the default acceleration.</source>
      <translation>内部实心填充的加速度。如果该值以百分比表示（例如100%），则将根据默认加速度进行计算。</translation>
    </message>
    <message>
      <source>Line width of internal solid infill. If expressed as a %, it will be computed over the nozzle diameter.</source>
      <translation>内部实心填充的线宽。如果以%表示，它将基于喷嘴直径来计算。</translation>
    </message>
    <message>
      <source>Internal solid infill pattern</source>
      <translation>内部实心填充图案</translation>
    </message>
    <message>
      <source>Speed of internal solid infill, not the top and bottom surface</source>
      <translation>内部实心填充的速度，不是顶面和底面。</translation>
    </message>
    <message>
      <source>Ironing angle</source>
      <translation>熨烫角度</translation>
    </message>
    <message>
      <source>The angle ironing is done at. A negative number disables this function and uses the default method.</source>
      <translation></translation>
    </message>
    <message>
      <source>Ironing flow</source>
      <translation>熨烫流量</translation>
    </message>
    <message>
      <source>The amount of material to extrude during ironing. Relative to flow of normal layer height. Too high value results in overextrusion on the surface</source>
      <translation>熨烫时相对正常层高流量的材料量。过高的数值将会导致表面材料过挤出。</translation>
    </message>
    <message>
      <source>Ironing Pattern</source>
      <translation>熨烫模式</translation>
    </message>
    <message>
      <source>The pattern that will be used when ironing</source>
      <translation>熨烫时将使用的图案</translation>
    </message>
    <message>
      <source>Ironing line spacing</source>
      <translation>熨烫间距</translation>
    </message>
    <message>
      <source>The distance between the lines of ironing</source>
      <translation>熨烫走线的间距</translation>
    </message>
    <message>
      <source>Ironing speed</source>
      <translation>熨烫速度</translation>
    </message>
    <message>
      <source>Print speed of ironing lines</source>
      <translation>熨烫的打印速度</translation>
    </message>
    <message>
      <source>Ironing Type</source>
      <translation>熨烫类型</translation>
    </message>
    <message>
      <source>Ironing is using small flow to print on same height of surface again to make flat surface more smooth. This setting controls which layer being ironed</source>
      <translation>熨烫指的是使用小流量在表面的同高度打印，进而是的平面更加光滑。这个设置用于设置哪些层进行熨烫。</translation>
    </message>
    <message>
      <source>No ironing</source>
      <translation>不熨烫</translation>
    </message>
    <message>
      <source>All solid layer</source>
      <translation>所有实心层</translation>
    </message>
    <message>
      <source>Top surfaces</source>
      <translation>顶面</translation>
    </message>
    <message>
      <source>Topmost surface</source>
      <translation>最顶面</translation>
    </message>
    <message>
      <source>Layer change G-code</source>
      <translation>换层G-code</translation>
    </message>
    <message>
      <source>This gcode part is inserted at every layer change after lift z</source>
      <translation>在每次换层抬升Z高度之后插入这段G-code。</translation>
    </message>
    <message>
      <source>Layer height</source>
      <translation>层高</translation>
    </message>
    <message>
      <source>Slicing height for each layer. Smaller layer height means more accurate and more printing time</source>
      <translation>每一层的切片高度。越小的层高意味着更高的精度和更长的打印时间。</translation>
    </message>
    <message>
      <source>Default line width if other line widths are set to 0. If expressed as a %, it will be computed over the nozzle diameter.</source>
      <translation>当线宽设置为0时走线的默认线宽。如果以%表示，它将基于喷嘴直径来计算。</translation>
    </message>
    <message>
      <source>End G-code when finish the whole printing</source>
      <translation>所有打印结束时的结尾G-code</translation>
    </message>
    <message>
      <source>Time to load new filament when switch filament. For statistics only</source>
      <translation>切换耗材丝时，加载新耗材丝所需的时间。只用于统计信息。</translation>
    </message>
    <message>
      <source>Maximum acceleration E</source>
      <translation>E最大加速度</translation>
    </message>
    <message>
      <source>Maximum acceleration of the E axis</source>
      <translation>E轴的最大加速度</translation>
    </message>
    <message>
      <source>Maximum acceleration for extruding</source>
      <translation>挤出最大加速度</translation>
    </message>
    <message>
      <source>Maximum acceleration for extruding (M204 P)</source>
      <translation>挤出时的最大加速度(M204 P)</translation>
    </message>
    <message>
      <source>Maximum acceleration for retracting</source>
      <translation>回抽最大加速度</translation>
    </message>
    <message>
      <source>Maximum acceleration for retracting (M204 R)</source>
      <translation>回抽最大加速度(M204 R)</translation>
    </message>
    <message>
      <source>Maximum acceleration for travel</source>
      <translation>空驶最大加速度</translation>
    </message>
    <message>
      <source>Maximum acceleration for travel (M204 T), it only applies to Marlin 2</source>
      <translation>最大空驶加速度（M204 T），仅适用于Marlin 2</translation>
    </message>
    <message>
      <source>Maximum acceleration X</source>
      <translation>X最大加速度</translation>
    </message>
    <message>
      <source>Maximum acceleration of the X axis</source>
      <translation>X轴的最大加速度</translation>
    </message>
    <message>
      <source>Maximum acceleration Y</source>
      <translation>Y最大加速度</translation>
    </message>
    <message>
      <source>Maximum acceleration of the Y axis</source>
      <translation>Y轴的最大加速度</translation>
    </message>
    <message>
      <source>Maximum acceleration Z</source>
      <translation>Z最大加速度</translation>
    </message>
    <message>
      <source>Maximum acceleration of the Z axis</source>
      <translation>Z轴的最大加速度</translation>
    </message>
    <message>
      <source>Maximum jerk E</source>
      <translation>E最大抖动</translation>
    </message>
    <message>
      <source>Maximum jerk of the E axis</source>
      <translation>E轴最大抖动</translation>
    </message>
    <message>
      <source>Maximum jerk X</source>
      <translation>X最大抖动</translation>
    </message>
    <message>
      <source>Maximum jerk of the X axis</source>
      <translation>X轴最大抖动</translation>
    </message>
    <message>
      <source>Maximum jerk Y</source>
      <translation>Y最大抖动</translation>
    </message>
    <message>
      <source>Maximum jerk of the Y axis</source>
      <translation>Y轴最大抖动</translation>
    </message>
    <message>
      <source>Maximum jerk Z</source>
      <translation>Z最大抖动</translation>
    </message>
    <message>
      <source>Maximum jerk of the Z axis</source>
      <translation>Z轴最大抖动</translation>
    </message>
    <message>
      <source>Maximum speed E</source>
      <translation>E最大速度</translation>
    </message>
    <message>
      <source>Maximum E speed</source>
      <translation>E最大速度</translation>
    </message>
    <message>
      <source>Maximum speed X</source>
      <translation>X最大速度</translation>
    </message>
    <message>
      <source>Maximum X speed</source>
      <translation>X最大速度</translation>
    </message>
    <message>
      <source>Maximum speed Y</source>
      <translation>Y最大速度</translation>
    </message>
    <message>
      <source>Maximum Y speed</source>
      <translation>Y最大速度</translation>
    </message>
    <message>
      <source>Maximum speed Z</source>
      <translation>Z最大速度</translation>
    </message>
    <message>
      <source>Maximum Z speed</source>
      <translation>Z最大速度</translation>
    </message>
    <message>
      <source>Minimum speed for extruding (M205 S)</source>
      <translation>最小挤出速度(M205 S)</translation>
    </message>
    <message>
      <source>Minimum travel speed (M205 T)</source>
      <translation>最小空驶速度(M205 T)</translation>
    </message>
    <message>
      <source>Pause G-code</source>
      <translation>暂停 G-code</translation>
    </message>
    <message>
      <source>This G-code will be used as a code for the pause print. User can insert pause G-code in gcode viewer</source>
      <translation>该G-code用于暂停打印。您可以在gcode预览中插入暂停G-code</translation>
    </message>
    <message>
      <source>Start G-code when start the whole printing</source>
      <translation>整个打印开始前的起始G-code</translation>
    </message>
    <message>
      <source>Time to unload old filament when switch filament. For statistics only</source>
      <translation>切换耗材丝时，卸载旧的耗材丝所需时间。只用于统计信息。</translation>
    </message>
    <message>
      <source>Make overhangs printable</source>
      <translation>悬垂可打印化</translation>
    </message>
    <message>
      <source>Modify the geometry to print overhangs without support material.</source>
      <translation>修改几何形状使得悬垂部分无需支撑材料或者搭桥打印。</translation>
    </message>
    <message>
      <source>Make overhangs printable - Maximum angle</source>
      <translation>悬垂可打印化的最大角度</translation>
    </message>
    <message>
      <source>Make overhangs printable - Hole area</source>
      <translation>最大孔洞面积</translation>
    </message>
    <message>
      <source>Maximum area of a hole in the base of the model before it's filled by conical material.A value of 0 will fill all the holes in the model base.</source>
      <translation>模型底部的孔洞在被圆锥形材料填充前所允许的最大面积。值为0将填充模型底部的所有孔洞。</translation>
    </message>
    <message>
      <source>Manual Filament Change</source>
      <translation>手动更换丝材</translation>
    </message>
    <message>
      <source>Enable this option to omit the custom Change filament G-code only at the beginning of the print. The tool change command (e.g., T0) will be skipped throughout the entire print. This is useful for manual multi-material printing, where we use M600/PAUSE to trigger the manual filament change action.</source>
      <translation>启用该选项可以在打印开始时省略自定义更换耗材丝的 G-code。整个打印过程中的工具头指令（如 T0）将会被跳过。这对于手动多材料打印十分有用，其将会使用 M600/PAUSE 指令来使您可以进行手动对耗材丝进行更换。</translation>
    </message>
    <message>
      <source>Max bridge length</source>
      <translation>最大桥接长度</translation>
    </message>
    <message>
      <source>Max length of bridges that don't need support. Set it to 0 if you want all bridges to be supported, and set it to a very large value if you don't want any bridges to be supported.</source>
      <translation>不需要支撑的桥接的最大长度。如果希望支持所有桥接，请将其设置为0；如果不希望支持任何桥接，请将其设置为非常大的值。</translation>
    </message>
    <message>
      <source>Max</source>
      <translation>最大</translation>
    </message>
    <message>
      <source>The largest printable layer height for extruder. Used tp limits the maximum layer hight when enable adaptive layer height</source>
      <translation>挤出头最大可打印的层高。用于限制开启自适应层高时的最大层高。</translation>
    </message>
    <message>
      <source>Avoid crossing wall - Max detour length</source>
      <translation>避免跨越外墙-最大绕行长度</translation>
    </message>
    <message>
      <source>Maximum detour distance for avoiding crossing wall. Don't detour if the detour distance is large than this value. Detour length could be specified either as an absolute value or as percentage (for example 50%) of a direct travel path. Zero to disable</source>
      <translation>避免跨越外墙时的最大绕行距离。当绕行距离比这个数值大时，此次空驶不绕行。绕行距离可表达为绝对值，或者相对直线空驶长度的百分比(例如50%)。0表示禁用</translation>
    </message>
    <message>
      <source>Extrusion rate smoothing</source>
      <translation>平滑挤出率</translation>
    </message>
    <message>
      <source>This parameter smooths out sudden extrusion rate changes that happen when the printer transitions from printing a high flow (high speed/larger width) extrusion to a lower flow (lower speed/smaller width) extrusion and vice versa.

It defines the maximum rate by which the extruded volumetric flow in mm3/sec can change over time. Higher values mean higher extrusion rate changes are allowed, resulting in faster speed transitions.

A value of 0 disables the feature.

For a high speed, high flow direct drive printer (like the Bambu lab or Voron) this value is usually not needed. However it can provide some marginal benefit in certain cases where feature speeds vary greatly. For example, when there are aggressive slowdowns due to overhangs. In these cases a high value of around 300-350mm3/s2 is recommended as this allows for just enough smoothing to assist pressure advance achieve a smoother flow transition.

For slower printers without pressure advance, the value should be set much lower. A value of 10-15mm3/s2 is a good starting point for direct drive extruders and 5-10mm3/s2 for Bowden style.

This feature is known as Pressure Equalizer in Prusa slicer.

Note: this parameter disables arc fitting.</source>
      <translation>此参数是打印机从打印高流量挤出（高速/较大宽度）向低流量挤出（低速/较小宽度）时用于突然变化挤出速率的平滑，反之亦然。
它定义了挤出体积流量（mm3/s）随时间变化的最大速率。更高的值意味着允许更高的挤出速率变化，从而产生更快速的过渡。

值为0将禁用该功能。

对于高速、高流量的近程挤出机（如Bambu或Voron）通常不需要该值。但是，在特征速度变化很大的某些情况下，它可以提供一些边际收益。例如，当由于悬垂而出现严重的减速时。在这些情况下，建议使用一个大的值大约300-350 m3/s2的，因为这刚好允许足够的平滑，以帮助压力提前实现更平滑的流量过渡。

对于没有压力提前的较慢打印机，该值应该设置得非常低。对于近程挤出机来说10-15 mm3/s2是一个很好值的起点，而对于远程挤出机来说是5-10 mm3/s2。

这个功能在Prusa切片机中被称为压力均衡器。

注意：此参数禁用圆弧拟合。</translation>
    </message>
    <message>
      <source>Smoothing segment length</source>
      <translation>平滑段长度</translation>
    </message>
    <message>
      <source>A lower value results in smoother extrusion rate transitions. However, this results in a significantly larger gcode file and more instructions for the printer to process.

Default value of 3 works well for most cases. If your printer is stuttering, increase this value to reduce the number of adjustments made

Allowed values: 1-5</source>
      <translation></translation>
    </message>
    <message>
      <source>Minimum wall width</source>
      <translation>墙最小线宽</translation>
    </message>
    <message>
      <source>Width of the wall that will replace thin features (according to the Minimum feature size) of the model. If the Minimum wall width is thinner than the thickness of the feature, the wall will become as thick as the feature itself. It's expressed as a percentage over nozzle diameter</source>
      <translation>用于替换模型细小特征（根据最小特征尺寸）的墙线宽。如果墙最小线宽小于最小特征的厚度，则墙将变得和特征本身一样厚。参数值表示为相对喷嘴直径的百分比</translation>
    </message>
    <message>
      <source>Minimum feature size</source>
      <translation>最小特征尺寸</translation>
    </message>
    <message>
      <source>Minimum thickness of thin features. Model features that are thinner than this value will not be printed, while features thicker than the Minimum feature size will be widened to the Minimum wall width. It's expressed as a percentage over nozzle diameter</source>
      <translation>薄壁特征的最小厚度。比这个数值还薄的特征将不被打印，而比最小特征厚度还厚的特征将被加宽到墙最小宽度。参数值表示为相对喷嘴直径的百分比</translation>
    </message>
    <message>
      <source>Min</source>
      <translation>最小</translation>
    </message>
    <message>
      <source>The lowest printable layer height for extruder. Used tp limits the minimum layer hight when enable adaptive layer height</source>
      <translation>挤出头最小可打印的层高。用于限制开启自适应层高时的最小层高。</translation>
    </message>
    <message>
      <source>One wall threshold</source>
      <translation>单层墙阈值</translation>
    </message>
    <message>
      <source>Minimum sparse infill threshold</source>
      <translation>稀疏填充最小阈值</translation>
    </message>
    <message>
      <source>Sparse infill area which is smaller than threshold value is replaced by internal solid infill</source>
      <translation>小于这个阈值的稀疏填充区域将会被内部实心填充替代。</translation>
    </message>
    <message>
      <source>Configuration notes</source>
      <translation>配置注释</translation>
    </message>
    <message>
      <source>You can put here your personal notes. This text will be added to the G-code header comments.</source>
      <translation>你可以把你的个人注释放在这里。该文本将添加到G-code首部注释中。</translation>
    </message>
    <message>
      <source>Nozzle diameter</source>
      <translation>喷嘴直径</translation>
    </message>
    <message>
      <source>Diameter of nozzle</source>
      <translation>喷嘴直径</translation>
    </message>
    <message>
      <source>Nozzle HRC</source>
      <translation>喷嘴洛氏硬度</translation>
    </message>
    <message>
      <source>The nozzle's hardness. Zero means no checking for nozzle's hardness during slicing.</source>
      <translation>喷嘴硬度。零值表示在切片时不检查喷嘴硬度。</translation>
    </message>
    <message>
      <source>Nozzle temperature for layers after the initial one</source>
      <translation>除首层外的其它层的喷嘴温度</translation>
    </message>
    <message>
      <source>Nozzle temperature to print initial layer when using this filament</source>
      <translation>打印首层时的喷嘴温度</translation>
    </message>
    <message>
      <source>Nozzle type</source>
      <translation>喷嘴类型</translation>
    </message>
    <message>
      <source>The metallic material of nozzle. This determines the abrasive resistance of nozzle, and what kind of filament can be printed</source>
      <translation>喷嘴的金属材料。这将决定喷嘴的耐磨性，以及可打印材料的种类</translation>
    </message>
    <message>
      <source>Brass</source>
      <translation>黄铜</translation>
    </message>
    <message>
      <source>Hardened steel</source>
      <translation>硬化钢</translation>
    </message>
    <message>
      <source>Stainless steel</source>
      <translation>不锈钢</translation>
    </message>
    <message>
      <source>Undefine</source>
      <translation>未定义</translation>
    </message>
    <message>
      <source>Nozzle volume</source>
      <translation>喷嘴内腔体积</translation>
    </message>
    <message>
      <source>Volume of nozzle between the cutter and the end of nozzle</source>
      <translation>从切刀位置到喷嘴尖端的内腔体积</translation>
    </message>
    <message>
      <source>Only one wall on first layer</source>
      <translation>首层仅单层墙</translation>
    </message>
    <message>
      <source>Use only one wall on first layer, to give more space to the bottom infill pattern</source>
      <translation>首层只使用单层墙，从而更多的空间能够使用底部填充图案</translation>
    </message>
    <message>
      <source>Only one wall on top surfaces</source>
      <translation>顶面单层墙</translation>
    </message>
    <message>
      <source>Use only one wall on flat top surface, to give more space to the top infill pattern</source>
      <translation>顶面只使用单层墙，从而更多的空间能够使用顶部填充图案</translation>
    </message>
    <message>
      <source>Outer wall</source>
      <translation>外墙</translation>
    </message>
    <message>
      <source>Acceleration of outer wall. Using a lower value can improve quality</source>
      <translation>外墙加速度。使用较小的值可以提高质量。</translation>
    </message>
    <message>
      <source>Jerk of outer walls</source>
      <translation>外墙抖动值</translation>
    </message>
    <message>
      <source>Line width of outer wall. If expressed as a %, it will be computed over the nozzle diameter.</source>
      <translation>外墙的线宽。如果以%表示，它将基于喷嘴直径来计算</translation>
    </message>
    <message>
      <source>Speed of outer wall which is outermost and visible. It's used to be slower than inner wall speed to get better quality.</source>
      <translation>外墙的打印速度。它通常使用比内壁速度慢的速度，以获得更好的质量。</translation>
    </message>
    <message>
      <source>Fan speed for overhang</source>
      <translation>悬垂风扇速度</translation>
    </message>
    <message>
      <source>Force part cooling fan to be this speed when printing bridge or overhang wall which has large overhang degree. Forcing cooling for overhang and bridge can get better quality for these part</source>
      <translation>当打印桥接和超过设定阈值的悬垂时，强制部件冷却风扇为设定的速度值。强制冷却能够使悬垂和桥接获得更好的打印质量</translation>
    </message>
    <message>
      <source>Cooling overhang threshold</source>
      <translation>冷却悬空阈值</translation>
    </message>
    <message>
      <source>Force cooling fan to be specific speed when overhang degree of printed part exceeds this value. Expressed as percentage which indicides how much width of the line without support from lower layer. 0% means forcing cooling for all outer wall no matter how much overhang degree</source>
      <translation>当打印件的悬空程度超过此值时，强制冷却风扇达到特定速度。用百分比表示，表明没有下层支撑的线的宽度是多少。0%%意味着无论悬垂程度如何，都要对所有外壁强制冷却。</translation>
    </message>
    <message>
      <source>Reverse on odd</source>
      <translation>反转奇数层悬垂方向</translation>
    </message>
    <message>
      <source>Extrude perimeters that have a part over an overhang in the reverse direction on odd layers. This alternating pattern can drastically improve steep overhang.</source>
      <translation>在奇数层，将悬垂的打印方向反转。这种交替的模式可以大大改善陡峭悬垂的打印质量。</translation>
    </message>
    <message>
      <source>Reverse threshold</source>
      <translation>反转阈值</translation>
    </message>
    <message>
      <source>Number of mm the overhang need to be for the reversal to be considered useful. Can be a % of the perimeter width.
Value 0 enables reversal on every odd layers regardless.</source>
      <translation>判定悬垂反转需要的值（毫米），可以是线宽的百分比。</translation>
    </message>
    <message>
      <source>Classic mode</source>
      <translation>经典模式</translation>
    </message>
    <message>
      <source>Enable this option to use classic mode</source>
      <translation>启用此选项以使用经典模式</translation>
    </message>
    <message>
      <source>Filament parking position</source>
      <translation>耗材停靠位置</translation>
    </message>
    <message>
      <source>Distance of the extruder tip from the position where the filament is parked when unloaded. This should match the value in printer firmware.</source>
      <translation>卸载时，挤出机齿尖与耗材停放位置的距离。这应该与打印机固件中的值相匹配。</translation>
    </message>
    <message>
      <source>Post-processing Scripts</source>
      <translation>后处理脚本</translation>
    </message>
    <message>
      <source>Precise wall(experimental)</source>
      <translation>精准外墙尺寸(试验)</translation>
    </message>
    <message>
      <source>Improve shell precision by adjusting outer wall spacing. This also improves layer consistency.</source>
      <translation>优化外墙刀路以提高外墙精度。这个优化同时减少层纹</translation>
    </message>
    <message>
      <source>Printer preset names</source>
      <translation>打印机预设名</translation>
    </message>
    <message>
      <source>Names of presets related to the physical printer</source>
      <translation>与物理打印机相关的预设名称</translation>
    </message>
    <message>
      <source>Pressure advance</source>
      <translation>压力提前</translation>
    </message>
    <message>
      <source>Pressure advance(Klipper) AKA Linear advance factor(Marlin)</source>
      <translation>压力提前(Klipper)或者线性提前(Marlin)</translation>
    </message>
    <message>
      <source>Width</source>
      <translation>宽度</translation>
    </message>
    <message>
      <source>Width of prime tower</source>
      <translation>擦拭塔宽度</translation>
    </message>
    <message>
      <source>Prime volume</source>
      <translation>清理量</translation>
    </message>
    <message>
      <source>The volume of material to prime extruder on tower.</source>
      <translation>擦拭塔上的清理量</translation>
    </message>
    <message>
      <source>Hostname, IP or URL</source>
      <translation>主机名，IP或者URL</translation>
    </message>
    <message>
      <source>Device UI</source>
      <translation>设备用户界面</translation>
    </message>
    <message>
      <source>Specify the URL of your device user interface if it's not same as print_host</source>
      <translation>如果打印机的设备用户界面的URL不同，请在此指定。</translation>
    </message>
    <message>
      <source>Print sequence</source>
      <translation>打印顺序</translation>
    </message>
    <message>
      <source>Print sequence, layer by layer or object by object</source>
      <translation>打印顺序，逐层打印或者逐件打印</translation>
    </message>
    <message>
      <source>By layer</source>
      <translation>逐层</translation>
    </message>
    <message>
      <source>By object</source>
      <translation>逐件</translation>
    </message>
    <message>
      <source>Printable area</source>
      <translation>可打印区域</translation>
    </message>
    <message>
      <source>Printable height</source>
      <translation>可打印高度</translation>
    </message>
    <message>
      <source>Maximum printable height which is limited by mechanism of printer</source>
      <translation>由打印机结构约束的最大可打印高度</translation>
    </message>
    <message>
      <source>Printer notes</source>
      <translation>打印机注释</translation>
    </message>
    <message>
      <source>You can put your notes regarding the printer here.</source>
      <translation>你可以把你关于打印机的注释放在这里。</translation>
    </message>
    <message>
      <source>Printer structure</source>
      <translation>打印机结构</translation>
    </message>
    <message>
      <source>The physical arrangement and components of a printing device</source>
      <translation>打印设备的物理结构和组件</translation>
    </message>
    <message>
      <source>CoreXY</source>
      <translation></translation>
    </message>
    <message>
      <source>Delta</source>
      <translation></translation>
    </message>
    <message>
      <source>Hbot</source>
      <translation></translation>
    </message>
    <message>
      <source>I3</source>
      <translation></translation>
    </message>
    <message>
      <source>API Key / Password</source>
      <translation>API秘钥/密码</translation>
    </message>
    <message>
      <source>Authorization Type</source>
      <translation>授权类型</translation>
    </message>
    <message>
      <source>API key</source>
      <translation>API秘钥</translation>
    </message>
    <message>
      <source>HTTP digest</source>
      <translation>HTTP摘要</translation>
    </message>
    <message>
      <source>HTTPS CA File</source>
      <translation>HTTPS CA文件</translation>
    </message>
    <message>
      <source>Custom CA certificate file can be specified for HTTPS OctoPrint connections, in crt/pem format. If left blank, the default OS CA certificate repository is used.</source>
      <translation>可以为HTTPS OctoPrint连接指定自定义CA证书文件，格式为crt/pem。如果留空，则使用默认的操作系统CA证书存储库。</translation>
    </message>
    <message>
      <source>Password</source>
      <translation>密码</translation>
    </message>
    <message>
      <source>Printer</source>
      <translation>打印机</translation>
    </message>
    <message>
      <source>Name of the printer</source>
      <translation>打印机名称</translation>
    </message>
    <message>
      <source>Ignore HTTPS certificate revocation checks</source>
      <translation>忽略HTTPS证书吊销检查</translation>
    </message>
    <message>
      <source>Ignore HTTPS certificate revocation checks in case of missing or offline distribution points. One may want to enable this option for self signed certificates if connection fails.</source>
      <translation>在缺少或离线分发点的情况下忽略HTTPS证书吊销检查。如果连接失败，可以启用此选项来处理自签名证书。</translation>
    </message>
    <message>
      <source>User</source>
      <translation>用户名</translation>
    </message>
    <message>
      <source>Purge in prime tower</source>
      <translation>冲刷进擦拭塔</translation>
    </message>
    <message>
      <source>Purge remaining filament into prime tower</source>
      <translation>冲刷剩余的耗材丝进入擦拭塔</translation>
    </message>
    <message>
      <source>Raft contact Z distance</source>
      <translation>筏层Z间距</translation>
    </message>
    <message>
      <source>Z gap between object and raft. Ignored for soluble interface</source>
      <translation>模型和筏层之间的Z间隙</translation>
    </message>
    <message>
      <source>Raft expansion</source>
      <translation>筏层扩展</translation>
    </message>
    <message>
      <source>Expand all raft layers in XY plane</source>
      <translation>在XY平面扩展所有筏层</translation>
    </message>
    <message>
      <source>Initial layer density</source>
      <translation>首层密度</translation>
    </message>
    <message>
      <source>Density of the first raft or support layer</source>
      <translation>筏和支撑的首层密度</translation>
    </message>
    <message>
      <source>Initial layer expansion</source>
      <translation>首层扩展</translation>
    </message>
    <message>
      <source>Expand the first raft or support layer to improve bed plate adhesion</source>
      <translation>扩展筏和支撑的首层可以改善和热床的粘接。</translation>
    </message>
    <message>
      <source>Raft layers</source>
      <translation>筏层</translation>
    </message>
    <message>
      <source>Object will be raised by this number of support layers. Use this function to avoid wrapping when print ABS</source>
      <translation>模型会在相应层数的支撑上抬高进行打印。使用该功能通常用于打印ABS时翘曲。</translation>
    </message>
    <message>
      <source>Avoid crossing wall</source>
      <translation>避免跨越外墙</translation>
    </message>
    <message>
      <source>Detour and avoid to travel across wall which may cause blob on surface</source>
      <translation>空驶时绕过外墙以避免在模型外观面产生斑点</translation>
    </message>
    <message>
      <source>Keep fan always on</source>
      <translation>保持风扇常开</translation>
    </message>
    <message>
      <source>If enable this setting, part cooling fan will never be stoped and will run at least at minimum speed to reduce the frequency of starting and stoping</source>
      <translation>如果勾选这个选项，部件冷却风扇将永远不会停止，并且会至少运行在最小风扇转速值以减少风扇的启停频次</translation>
    </message>
    <message>
      <source>Reduce infill retraction</source>
      <translation>减小填充回抽</translation>
    </message>
    <message>
      <source>Don't retract when the travel is in infill area absolutely. That means the oozing can't been seen. This can reduce times of retraction for complex model and save printing time, but make slicing and G-code generating slower</source>
      <translation>当空驶完全在填充区域内时不触发回抽。这意味着即使漏料也是不可见的。对于复杂模型，该设置能够减少回抽次数以及打印时长，但是会造成G-code生成变慢</translation>
    </message>
    <message>
      <source>Required nozzle HRC</source>
      <translation>喷嘴硬度要求</translation>
    </message>
    <message>
      <source>Minimum HRC of nozzle required to print the filament. Zero means no checking of nozzle's HRC.</source>
      <translation>打印此材料的所需的最小喷嘴硬度。零值表示不检查喷嘴硬度。</translation>
    </message>
    <message>
      <source>Resolution</source>
      <translation>分辨率</translation>
    </message>
    <message>
      <source>G-code path is genereated after simplifing the contour of model to avoid too much points and gcode lines in gcode file. Smaller value means higher resolution and more time to slice</source>
      <translation>为了避免G-code文件中过密集的点和走线，G-code走线通常是在简化模型的外轮廓之后生成。越小的数值代表更高的分辨率，同时需要更长的切片时间。</translation>
    </message>
    <message>
      <source>When the retraction is compensated after changing tool, the extruder will push this additional amount of filament.</source>
      <translation>当换色后回抽被补偿时，挤出机将推入额外数量的耗材丝。</translation>
    </message>
    <message>
      <source>Role base wipe speed</source>
      <translation>自动擦拭速度</translation>
    </message>
    <message>
      <source>The wipe speed is determined by the speed of the current extrusion role.e.g. if a wipe action is executed immediately following an outer wall extrusion, the speed of the outer wall extrusion will be utilized for the wipe action.</source>
      <translation></translation>
    </message>
    <message>
      <source>Scan first layer</source>
      <translation>首层扫描</translation>
    </message>
    <message>
      <source>Enable this to enable the camera on printer to check the quality of first layer</source>
      <translation>开启这个设置将打开打印机上的摄像头用于检查首层打印质量。</translation>
    </message>
    <message>
      <source>Seam gap</source>
      <translation>接缝间隔</translation>
    </message>
    <message>
      <source>In order to reduce the visibility of the seam in a closed loop extrusion, the loop is interrupted and shortened by a specified amount.
This amount can be specified in millimeters or as a percentage of the current extruder diameter. The default value for this parameter is 10%.</source>
      <translation></translation>
    </message>
    <message>
      <source>Seam position</source>
      <translation>接缝位置</translation>
    </message>
    <message>
      <source>The start position to print each part of outer wall</source>
      <translation>开始打印外墙的位置</translation>
    </message>
    <message>
      <source>Aligned</source>
      <translation>对齐</translation>
    </message>
    <message>
      <source>Back</source>
      <translation>背面</translation>
    </message>
    <message>
      <source>Nearest</source>
      <translation>最近</translation>
    </message>
    <message>
      <source>Random</source>
      <translation>随机</translation>
    </message>
    <message>
      <source>Supports silent mode</source>
      <translation>支持静音模式</translation>
    </message>
    <message>
      <source>Whether the machine supports silent mode in which machine use lower acceleration to print</source>
      <translation>机器是否支持使用低加速度打印的静音模式。</translation>
    </message>
    <message>
      <source>Single Extruder Multi Material</source>
      <translation>单挤出机多材料</translation>
    </message>
    <message>
      <source>Use single nozzle to print multi filament</source>
      <translation>使用单喷嘴打印多耗材</translation>
    </message>
    <message>
      <source>Prime all printing extruders</source>
      <translation>所有挤出机画线</translation>
    </message>
    <message>
      <source>If enabled, all printing extruders will be primed at the front edge of the print bed at the start of the print.</source>
      <translation>如果启用，所有挤出机将在打印开始时在床前画线</translation>
    </message>
    <message>
      <source>Skirt distance</source>
      <translation>Skirt距离</translation>
    </message>
    <message>
      <source>Distance from skirt to brim or object</source>
      <translation>从skirt到模型或者brim的距离</translation>
    </message>
    <message>
      <source>Skirt height</source>
      <translation>Skirt高度</translation>
    </message>
    <message>
      <source>How many layers of skirt. Usually only one layer</source>
      <translation>skirt有多少层。通常只有一层</translation>
    </message>
    <message>
      <source>Skirt loops</source>
      <translation>Skirt圈数</translation>
    </message>
    <message>
      <source>Number of loops for the skirt. Zero means disabling skirt</source>
      <translation>skirt的圈数。0表示关闭skirt。</translation>
    </message>
    <message>
      <source>Skirt speed</source>
      <translation>Skirt速度</translation>
    </message>
    <message>
      <source>Speed of skirt, in mm/s. Zero means use default layer extrusion speed.</source>
      <translation>skirt速度，单位为mm/s。0表示使用默认的层挤出速度。</translation>
    </message>
    <message>
      <source>Slice gap closing radius</source>
      <translation>切片间隙闭合半径</translation>
    </message>
    <message>
      <source>Cracks smaller than 2x gap closing radius are being filled during the triangle mesh slicing. The gap closing operation may reduce the final print resolution, therefore it is advisable to keep the value reasonably low.</source>
      <translation>在三角形网格切片过程中，小于2倍间隙闭合半径的裂纹将被填充。间隙闭合操作可能会降低最终打印分辨率，因此建议降值保持在合理的较低水平</translation>
    </message>
    <message>
      <source>Slicing Mode</source>
      <translation>切片模式</translation>
    </message>
    <message>
      <source>Use &quot;Even-odd&quot; for 3DLabPrint airplane models. Use &quot;Close holes&quot; to close all holes in the model.</source>
      <translation>对3DLabPrint的飞机模型使用 &quot;奇偶&quot;。使用 &quot;闭孔 &quot;来关闭模型上的所有孔。</translation>
    </message>
    <message>
      <source>Close holes</source>
      <translation>闭孔</translation>
    </message>
    <message>
      <source>Even-odd</source>
      <translation>奇偶</translation>
    </message>
    <message>
      <source>Regular</source>
      <translation>常规</translation>
    </message>
    <message>
      <source>Slow printing down for better layer cooling</source>
      <translation>降低打印速度 以得到更好的冷却</translation>
    </message>
    <message>
      <source>Enable this option to slow printing speed down to make the final layer time not shorter than the layer time threshold in &quot;Max fan speed threshold&quot;, so that layer can be cooled for longer time. This can improve the cooling quality for needle and small details</source>
      <translation>勾选这个选项，将降低打印速度，使得最终的层打印时间不小于&quot;最大风扇速度阈值&quot;里的层时间阈值，从而使得该层获得更久的冷却。这能够改善尖顶和小细节的冷却效果</translation>
    </message>
    <message>
      <source>The printing speed in exported gcode will be slowed down, when the estimated layer time is shorter than this value, to get better cooling for these layers</source>
      <translation>当层预估打印时间小于这个数值时，打印速度将会降低，从而获得更好的冷却效果。</translation>
    </message>
    <message>
      <source>Number of slow layers</source>
      <translation>慢速打印层数</translation>
    </message>
    <message>
      <source>The first few layers are printed slower than normal. The speed is gradually increased in a linear fashion over the specified number of layers.</source>
      <translation>减慢前几层的打印速度。打印速度会逐渐加速到满速</translation>
    </message>
    <message>
      <source>Min print speed</source>
      <translation>最小打印速度</translation>
    </message>
    <message>
      <source>The minimum printing speed for the filament when slow down for better layer cooling is enabled, when printing overhangs and when feature speeds are not specified explicitly.</source>
      <translation></translation>
    </message>
    <message>
      <source>Slow down for curled perimeters</source>
      <translation>翘边降速</translation>
    </message>
    <message>
      <source>Enable this option to slow printing down in areas where potential curled perimeters may exist</source>
      <translation>启用这个选项，降低可能存在卷曲部位的打印速度</translation>
    </message>
    <message>
      <source>Small perimeters</source>
      <translation>微小部位</translation>
    </message>
    <message>
      <source>This separate setting will affect the speed of perimeters having radius &lt;= small_perimeter_threshold (usually holes). If expressed as percentage (for example: 80%) it will be calculated on the outer wall speed setting above. Set to zero for auto.</source>
      <translation>此设置将影响半径小于等于small_perimeter_threshold（通常是孔洞）的围墙的速度。如果以百分比表示（例如：80%），则将根据上面的外壁速度设置进行计算。设置为零时为自动。</translation>
    </message>
    <message>
      <source>Small perimeters threshold</source>
      <translation>微小部位周长阈值</translation>
    </message>
    <message>
      <source>This sets the threshold for small perimeter length. Default threshold is 0mm</source>
      <translation>这将设置微小部位周长的阈值。默认阈值为0mm</translation>
    </message>
    <message>
      <source>Sparse infill</source>
      <translation>稀疏填充</translation>
    </message>
    <message>
      <source>Acceleration of sparse infill. If the value is expressed as a percentage (e.g. 100%), it will be calculated based on the default acceleration.</source>
      <translation>稀疏填充的加速度。如果该值表示为百分比（例如100%），则将根据默认加速度进行计算。</translation>
    </message>
    <message>
      <source>Sparse infill density</source>
      <translation>稀疏填充密度</translation>
    </message>
    <message>
      <source>Density of internal sparse infill, 100% means solid throughout</source>
      <translation>稀疏填充密度, 100%% 意味着完全实心。</translation>
    </message>
    <message>
      <source>Filament to print internal sparse infill.</source>
      <translation>打印内部稀疏填充的耗材丝</translation>
    </message>
    <message>
      <source>Line width of internal sparse infill. If expressed as a %, it will be computed over the nozzle diameter.</source>
      <translation>内部稀疏填充的线宽。如果以%表示，它将基于喷嘴直径来计算。</translation>
    </message>
    <message>
      <source>Sparse infill pattern</source>
      <translation>稀疏填充图案</translation>
    </message>
    <message>
      <source>Line pattern for internal sparse infill</source>
      <translation>内部稀疏填充的走线图案</translation>
    </message>
    <message>
      <source>3D Honeycomb</source>
      <translation>3D 蜂窝</translation>
    </message>
    <message>
      <source>Adaptive Cubic</source>
      <translation>自适应立方体</translation>
    </message>
    <message>
      <source>Cubic</source>
      <translation>立方体</translation>
    </message>
    <message>
      <source>Grid</source>
      <translation>网格</translation>
    </message>
    <message>
      <source>Gyroid</source>
      <translation>螺旋体</translation>
    </message>
    <message>
      <source>Honeycomb</source>
      <translation>蜂窝</translation>
    </message>
    <message>
      <source>Lightning</source>
      <translation>闪电</translation>
    </message>
    <message>
      <source>Line</source>
      <translation>线</translation>
    </message>
    <message>
      <source>Support Cubic</source>
      <translation>支撑立方体</translation>
    </message>
    <message>
      <source>Tri-hexagon</source>
      <translation>内六边形</translation>
    </message>
    <message>
      <source>Triangles</source>
      <translation>三角形</translation>
    </message>
    <message>
      <source>Speed of internal sparse infill</source>
      <translation>内部稀疏填充的打印速度</translation>
    </message>
    <message>
      <source>Spiral vase</source>
      <translation>旋转花瓶</translation>
    </message>
    <message>
      <source>Spiralize smooths out the z moves of the outer contour. And turns a solid model into a single walled print with solid bottom layers. The final generated model has no seam</source>
      <translation>沿着对象的外轮廓螺旋上升，将实体模型转变为只有底面实心层和侧面单层墙壁的打印。最后生成的打印件没有接缝。</translation>
    </message>
    <message>
      <source>Staggered inner seams</source>
      <translation>交错的内墙接缝</translation>
    </message>
    <message>
      <source>This option causes the inner seams to be shifted backwards based on their depth, forming a zigzag pattern.</source>
      <translation>此选项会根据内墙深度使接缝向后移动，形成锯齿形模式。</translation>
    </message>
    <message>
      <source>Temperature variation</source>
      <translation>软化温度</translation>
    </message>
    <message>
      <source>Start end points</source>
      <translation>起始终止点</translation>
    </message>
    <message>
      <source>The start and end points which is from cutter area to garbage can.</source>
      <translation>从切割区域到垃圾桶的起始和结束点。</translation>
    </message>
    <message>
      <source>Support air filtration</source>
      <translation>支持空气过滤</translation>
    </message>
    <message>
      <source>Enable this if printer support air filtration
G-code command: M106 P3 S(0-255)</source>
      <translation>如果打印机支持空气过滤/排气风扇，可以开启此选项
G-code指令：M106 P3 S(0-255)</translation>
    </message>
    <message>
      <source>Pattern angle</source>
      <translation>模式角度</translation>
    </message>
    <message>
      <source>Use this setting to rotate the support pattern on the horizontal plane.</source>
      <translation>设置支撑图案在水平面的旋转角度。</translation>
    </message>
    <message>
      <source>Base pattern</source>
      <translation>支撑主体图案</translation>
    </message>
    <message>
      <source>Line pattern of support</source>
      <translation>支撑走线图案</translation>
    </message>
    <message>
      <source>Hollow</source>
      <translation>空心</translation>
    </message>
    <message>
      <source>Rectilinear grid</source>
      <translation>直线网格</translation>
    </message>
    <message>
      <source>Base pattern spacing</source>
      <translation>主体图案线距</translation>
    </message>
    <message>
      <source>Spacing between support lines</source>
      <translation>支撑线距</translation>
    </message>
    <message>
      <source>Bottom interface spacing</source>
      <translation>底部接触面线距</translation>
    </message>
    <message>
      <source>Spacing of bottom interface lines. Zero means solid interface</source>
      <translation>底部接触面走线的线距。0表示实心接触面。</translation>
    </message>
    <message>
      <source>Bottom Z distance</source>
      <translation>底部Z距离</translation>
    </message>
    <message>
      <source>The z gap between the bottom support interface and object</source>
      <translation>支撑生成于模型表面时，支撑面底部和模型之间的z间隙</translation>
    </message>
    <message>
      <source>Support control chamber temperature</source>
      <translation>支持仓温控制</translation>
    </message>
    <message>
      <source>This option is enabled if machine support controlling chamber temperature
G-code command: M141 S(0-255)</source>
      <translation></translation>
    </message>
    <message>
      <source>Support critical regions only</source>
      <translation>仅支撑关键区域</translation>
    </message>
    <message>
      <source>Only create support for critical regions including sharp tail, cantilever, etc.</source>
      <translation>仅对关键区域生成支撑，包括尖尾、悬臂等。</translation>
    </message>
    <message>
      <source>Normal Support expansion</source>
      <translation>普通支撑拓展</translation>
    </message>
    <message>
      <source>Expand (+) or shrink (-) the horizontal span of normal support</source>
      <translation>在水平方向对普通支撑进行拓展（+）或收缩（-）</translation>
    </message>
    <message>
      <source>Support/raft base</source>
      <translation>支撑/筏层主体</translation>
    </message>
    <message>
      <source>Filament to print support base and raft. &quot;Default&quot; means no specific filament for support and current filament is used</source>
      <translation>打印支撑主体和筏层的耗材丝。&quot;缺省&quot;代表不指定特定的耗材丝，并使用当前耗材</translation>
    </message>
    <message>
      <source>Bottom interface layers</source>
      <translation>底部接触面层数</translation>
    </message>
    <message>
      <source>Support/raft interface</source>
      <translation>支撑/筏层界面</translation>
    </message>
    <message>
      <source>Filament to print support interface. &quot;Default&quot; means no specific filament for support interface and current filament is used</source>
      <translation>打印支撑接触面的耗材丝。&quot;缺省&quot;代表不指定特定的耗材丝，并使用当前耗材</translation>
    </message>
    <message>
      <source>Interface use loop pattern</source>
      <translation>接触面采用圈形走线。</translation>
    </message>
    <message>
      <source>Cover the top contact layer of the supports with loops. Disabled by default.</source>
      <translation>使用圈形走线覆盖顶部接触面。默认关闭。</translation>
    </message>
    <message>
      <source>Interface pattern</source>
      <translation>支撑面图案</translation>
    </message>
    <message>
      <source>Line pattern of support interface. Default pattern for non-soluble support interface is Rectilinear, while default pattern for soluble support interface is Concentric</source>
      <translation>支撑接触面的走线图案。非可溶支撑接触面的缺省图案为直线，可溶支撑接触面的缺省图案为同心。</translation>
    </message>
    <message>
      <source>Rectilinear Interlaced</source>
      <translation>交叠的直线</translation>
    </message>
    <message>
      <source>Top interface spacing</source>
      <translation>顶部接触面线距</translation>
    </message>
    <message>
      <source>Spacing of interface lines. Zero means solid interface</source>
      <translation>接触面的线距。0代表实心接触面。</translation>
    </message>
    <message>
      <source>Support interface</source>
      <translation>支撑面</translation>
    </message>
    <message>
      <source>Speed of support interface</source>
      <translation>支撑面速度</translation>
    </message>
    <message>
      <source>Top interface layers</source>
      <translation>顶部接触面层数</translation>
    </message>
    <message>
      <source>Number of top interface layers</source>
      <translation>顶部接触面层数</translation>
    </message>
    <message>
      <source>Line width of support. If expressed as a %, it will be computed over the nozzle diameter.</source>
      <translation>支撑的线宽。如果以%表示，它将基于喷嘴直径来计算。</translation>
    </message>
    <message>
      <source>Support interface fan speed</source>
      <translation>支撐接触面风扇</translation>
    </message>
    <message>
      <source>This fan speed is enforced during all support interfaces, to be able to weaken their bonding with a high fan speed.
Set to -1 to disable this override.
Can only be overriden by disable_fan_first_layers.</source>
      <translation>此风扇速度在所有支撑接触层打印期间强制执行</translation>
    </message>
    <message>
      <source>Support/object xy distance</source>
      <translation>支撑/模型xy间距</translation>
    </message>
    <message>
      <source>XY separation between an object and its support</source>
      <translation>模型和支撑之间XY分离距离</translation>
    </message>
    <message>
      <source>On build plate only</source>
      <translation>仅在构建板生成</translation>
    </message>
    <message>
      <source>Don't create support on model surface, only on build plate</source>
      <translation>不在模型表面上生成支撑，只在热床上生成。</translation>
    </message>
    <message>
      <source>Remove small overhangs</source>
      <translation>移除小悬空</translation>
    </message>
    <message>
      <source>Remove small overhangs that possibly need no supports.</source>
      <translation>移除可能并不需要支撑的小悬空。</translation>
    </message>
    <message>
      <source>Speed of support</source>
      <translation>支撑打印速度</translation>
    </message>
    <message>
      <source>Style</source>
      <translation>样式</translation>
    </message>
    <message>
      <source>Style and shape of the support. For normal support, projecting the supports into a regular grid will create more stable supports (default), while snug support towers will save material and reduce object scarring.
For tree support, slim and organic style will merge branches more aggressively and save a lot of material (default organic), while hybrid style will create similar structure to normal support under large flat overhangs.</source>
      <translation>支撑的样式和形状。为了正常的支持，将支撑物投影到常规网格中将创建更稳定的支撑（默认），而舒适的支撑塔将节省材料并减少对象疤痕。
对于树的支撑，苗条和有机样式将更加积极地合并分支并节省大量材料（默认有机），而混合风格将在大型平坦的悬垂物下创建与正常支撑相似的结构。</translation>
    </message>
	    <message>
      <source>Connect an infill line to an internal perimeter with a short segment of an additional perimeter. If expressed as percentage (example: 15%) it is calculated over infill extrusion width. Orca Slicer tries to connect two close infill lines to a short perimeter segment. If no such perimeter segment shorter than infill_anchor_max is found, the infill line is connected to a perimeter segment at just one side and the length of the perimeter segment taken is limited to this parameter, but no longer than anchor_length_max.
Set this parameter to zero to disable anchoring perimeters connected to a single infill line.</source>
      <translation>用附加周长的一小段将填充线连接到内部周长。如果以百分比（例如：15%）表示，则计算填充拉伸宽度。OrcaSlicer 试图将两条紧密的填充线连接到一个短的周长段。如果找不到短于“填充”和“锚点”最大值的周长线段，则填充线仅在一侧连接到周长线段，并且所取周长线段的长度仅限于此参数，但不超过“锚点长度”最大值。将此参数设置为零，以禁用连接到单个填充线的锚点周长。</translation>
    </message>
    <message>
        <source>Connect an infill line to an internal perimeter with a short segment of an additional perimeter. If expressed as percentage (example: 15%) it is calculated over infill extrusion width. Orca Slicer tries to connect two close infill lines to a short perimeter segment. If no such perimeter segment shorter than this parameter is found, the infill line is connected to a perimeter segment at just one side and the length of the perimeter segment taken is limited to infill_anchor, but no longer than this parameter.
If set to 0, the old algorithm for infill connection will be used, it should create the same result as with 1000 &amp; 0.</source>
        <translation>用附加周长的一小段将填充线连接到内部周长。如果以百分比（例如：15%）表示，则计算填充拉伸宽度。OrcaSlicer 试图将两条紧密的填充线连接到一个短的周长段。如果找不到比此参数短的周长线段，则填充线仅在一侧连接到周长线段，并且所采用的周长线段的长度仅限于 infl_anchor，但不超过此参数。将此参数设置为零以禁用锚点。</translation>
    </message>
	    <message>
        <source>Line pattern of internal solid infill. if the detect narrow internal solid infill be enabled, the concentric pattern will be used for the small area.</source>
        <translation>内部实心填充的线型图案。如果启用了检测狭窄的内部实心填充，将使用同心圆图案来填充小区域。</translation>
    </message>
    <message>
      <source>Organic</source>
      <translation>有机树</translation>
    </message>
    <message>
      <source>Snug</source>
      <translation>紧贴</translation>
    </message>
    <message>
      <source>Tree Hybrid</source>
      <translation>混合树</translation>
    </message>
    <message>
      <source>Tree Slim</source>
      <translation>苗条树</translation>
    </message>
    <message>
      <source>Tree Strong</source>
      <translation>粗壮树</translation>
    </message>
    <message>
      <source>Threshold angle</source>
      <translation>阈值角度</translation>
    </message>
    <message>
      <source>Support will be generated for overhangs whose slope angle is below the threshold.</source>
      <translation>将会为悬垂角度低于阈值的模型表面生成支撑。</translation>
    </message>
    <message>
      <source>Top Z distance</source>
      <translation>顶部Z距离</translation>
    </message>
    <message>
      <source>The z gap between the top support interface and object</source>
      <translation>支撑顶部和模型之间的z间隙</translation>
    </message>
    <message>
      <source>normal(auto) and tree(auto) is used to generate support automatically. If normal(manual) or tree(manual) is selected, only support enforcers are generated</source>
      <translation>普通（自动）和树状（自动）用于自动生成支撑体。如果选择普通（手动）或树状（手动），仅会在支撑强制面上生成支撑。</translation>
    </message>
    <message>
      <source>normal(auto)</source>
      <translation>普通(自动)</translation>
    </message>
    <message>
      <source>normal(manual)</source>
      <translation>普通(手动)</translation>
    </message>
    <message>
      <source>tree(auto)</source>
      <translation>树状(自动)</translation>
    </message>
    <message>
      <source>tree(manual)</source>
      <translation>树状(手动)</translation>
    </message>
    <message>
      <source>Softening temperature</source>
      <translation>软化温度</translation>
    </message>
    <message>
      <source>The material softens at this temperature, so when the bed temperature is equal to or greater than it, it's highly recommended to open the front door and/or remove the upper glass to avoid cloggings.</source>
      <translation>材料在这个温度下会软化，因此当热床温度等于或高于这个温度时，强烈建议打开前门和/或去除上玻璃以避免堵塞。</translation>
    </message>
    <message>
      <source>Custom G-code</source>
      <translation>自定义 G-code</translation>
    </message>
    <message>
      <source>This G-code will be used as a custom code</source>
      <translation>该G-code是定制化指令</translation>
    </message>
    <message>
      <source>Bed temperature for layers except the initial one. Value 0 means the filament does not support to print on the Textured PEI Plate</source>
      <translation>非首层热床温度。0值表示这个耗材丝不支持纹理PEI热床</translation>
    </message>
    <message>
      <source>Bed temperature of the initial layer. Value 0 means the filament does not support to print on the Textured PEI Plate</source>
      <translation>首层热床温度。0值表示这个耗材丝不支持纹理PEI热床</translation>
    </message>
    <message>
      <source>Thick bridges</source>
      <translation>厚桥</translation>
    </message>
    <message>
      <source>If enabled, bridges are more reliable, can bridge longer distances, but may look worse. If disabled, bridges look better but are reliable just for shorter bridged distances.</source>
      <translation>如果启用，桥接会更可靠，可以桥接更长的距离，但可能看起来更糟。如果关闭，桥梁看起来更好，但只适用于较短的桥接距离。</translation>
    </message>
    <message>
      <source>Thick internal bridges</source>
      <translation>内部搭桥用厚桥</translation>
    </message>
    <message>
      <source>If enabled, thick internal bridges will be used. It's usually recommended to have this feature turned on. However, consider turning it off if you are using large nozzles.</source>
      <translation>如果启用，将使用厚内部桥接。通常建议打开此功能。但是，如果您使用大喷嘴，请考虑关闭它。</translation>
    </message>
    <message>
      <source>G-code thumbnails</source>
      <translation>G-code缩略图尺寸</translation>
    </message>
    <message>
      <source>Picture sizes to be stored into a .gcode and .sl1 / .sl1s files, in the following format: &quot;XxY, XxY, ...&quot;</source>
      <translation>将被存储到.gcode和.sl1/.sl1s文件中的图片尺寸，格式如下：&quot;XxY, XxY, ...&quot;</translation>
    </message>
    <message>
      <source>Format of G-code thumbnails</source>
      <translation>G-code缩略图的格式</translation>
    </message>
    <message>
      <source>Format of G-code thumbnails: PNG for best quality, JPG for smallest size, QOI for low memory firmware</source>
      <translation>G-Code 缩略图格式: PNG 质量最佳，JPG 尺寸最小，QOI 用于低内存固件</translation>
    </message>
    <message>
      <source>Time cost</source>
      <translation>时间花费</translation>
    </message>
    <message>
      <source>The printer cost per hour</source>
      <translation>打印机每小时的成本</translation>
    </message>
    <message>
      <source>Time lapse G-code</source>
      <translation>延时摄影G-code</translation>
    </message>
    <message>
      <source>Timelapse</source>
      <translation>延时摄影</translation>
    </message>
    <message>
      <source>If smooth or traditional mode is selected, a timelapse video will be generated for each print. After each layer is printed, a snapshot is taken with the chamber camera. All of these snapshots are composed into a timelapse video when printing completes. If smooth mode is selected, the toolhead will move to the excess chute after each layer is printed and then take a snapshot. Since the melt filament may leak from the nozzle during the process of taking a snapshot, prime tower is required for smooth mode to wipe nozzle.</source>
      <translation>如果启用平滑模式或者传统模式，将在每次打印时生成延时摄影视频。打印完每层后，将用内置相机拍摄快照。打印完成后，所有这些快照会组合成一个延时视频。如果启用平滑模式，打印完每层后，工具头将移动到吐料槽，然后拍摄快照。由于平滑模式在拍摄快照的过程中熔丝可能会从喷嘴中泄漏，因此需要使用擦拭塔进行喷嘴擦拭。</translation>
    </message>
    <message>
      <source>Traditional</source>
      <translation>传统模式</translation>
    </message>
    <message>
      <source>Smooth</source>
      <translation>平滑模式</translation>
    </message>
    <message>
      <source>Top shell layers</source>
      <translation>顶部壳体层数</translation>
    </message>
    <message>
      <source>This is the number of solid layers of top shell, including the top surface layer. When the thickness calculated by this value is thinner than top shell thickness, the top shell layers will be increased</source>
      <translation>顶部壳体实心层层数，包括顶面。当由该层数计算的厚度小于顶部壳体厚度，切片时会增加顶部壳体的层数</translation>
    </message>
    <message>
      <source>Top shell thickness</source>
      <translation>顶部壳体厚度</translation>
    </message>
    <message>
      <source>The number of top solid layers is increased when slicing if the thickness calculated by top shell layers is thinner than this value. This can avoid having too thin shell when layer height is small. 0 means that this setting is disabled and thickness of top shell is absolutely determained by top shell layers</source>
      <translation>如果由顶部壳体层数算出的厚度小于这个数值，那么切片时将自动增加顶部壳体层数。这能够避免当层高很小时，顶部壳体过薄。0表示关闭这个设置，同时顶部壳体的厚度完全由顶部壳体层数决定</translation>
    </message>
    <message>
      <source>Top surface flow ratio</source>
      <translation>顶部表面流量比例</translation>
    </message>
    <message>
      <source>This factor affects the amount of material for top solid infill. You can decrease it slightly to have smooth surface finish</source>
      <translation>稍微减小这个数值（比如0.97）可以来改善顶面的光滑程度。</translation>
    </message>
    <message>
      <source>Top surface</source>
      <translation>顶面</translation>
    </message>
    <message>
      <source>Acceleration of top surface infill. Using a lower value may improve top surface quality</source>
      <translation>顶面填充的加速度。使用较低值可能会改善顶面质量</translation>
    </message>
    <message>
      <source>Jerk for top surface</source>
      <translation>顶面抖动值</translation>
    </message>
    <message>
      <source>Line width for top surfaces. If expressed as a %, it will be computed over the nozzle diameter.</source>
      <translation>顶面的线宽。如果以%表示，它将基于喷嘴直径来计算。</translation>
    </message>
    <message>
      <source>Top surface pattern</source>
      <translation>顶面图案</translation>
    </message>
    <message>
      <source>Line pattern of top surface infill</source>
      <translation>顶面填充的走线图案</translation>
    </message>
    <message>
      <source>Speed of top surface infill which is solid</source>
      <translation>顶面实心填充的速度</translation>
    </message>
    <message>
      <source>Travel</source>
      <translation>空驶</translation>
    </message>
    <message>
      <source>Acceleration of travel moves</source>
      <translation>空驶加速度</translation>
    </message>
    <message>
      <source>Jerk for travel</source>
      <translation>空驶抖动值</translation>
    </message>
    <message>
      <source>Speed of travel which is faster and without extrusion</source>
      <translation>空驶的速度。空驶是无挤出量的快速移动。</translation>
    </message>
    <message>
      <source>Adaptive layer height</source>
      <translation>自适应层高</translation>
    </message>
    <message>
      <source>Enabling this option means the height of  tree support layer except the first will be automatically calculated </source>
      <translation>启用此选项将自动计算（除第一层外）树状支撑的层高。</translation>
    </message>
    <message>
      <source>Preferred Branch Angle</source>
      <translation>首选分支角度</translation>
    </message>
    <message>
      <source>The preferred angle of the branches, when they do not have to avoid the model. Use a lower angle to make them more vertical and more stable. Use a higher angle for branches to merge faster.</source>
      <translation>分支的首选角度，每当它们不必避开模型之时。使用小的角度使之更垂直、更稳定。使用大的角度使之更快地合并。</translation>
    </message>
    <message>
      <source>Auto brim width</source>
      <translation>自动裙边宽度</translation>
    </message>
    <message>
      <source>Enabling this option means the width of the brim for tree support will be automatically calculated</source>
      <translation>启用此选项意味着树状支撑的裙边宽度将自动计算自动计算</translation>
    </message>
    <message>
      <source>Tree support branch angle</source>
      <translation>树状支撑分支角度</translation>
    </message>
    <message>
      <source>This setting determines the maximum overhang angle that t he branches of tree support allowed to make.If the angle is increased, the branches can be printed more horizontally, allowing them to reach farther.</source>
      <translation>此设置确定了允许树状支撑的最大悬垂角度。如果角度增加，可以更水平地打印分支，使它们可以到达更远的地方。</translation>
    </message>
    <message>
      <source>Tree support branch diameter</source>
      <translation>树状支撑分支直径</translation>
    </message>
    <message>
      <source>This setting determines the initial diameter of support nodes.</source>
      <translation>此设置确定了树状支撑节点的初始直径。</translation>
    </message>
    <message>
      <source>Branch Diameter Angle</source>
      <translation>分支直径的角度</translation>
    </message>
    <message>
      <source>The angle of the branches' diameter as they gradually become thicker towards the bottom. An angle of 0 will cause the branches to have uniform thickness over their length. A bit of an angle can increase stability of the organic support.</source>
      <translation>分支直径的角度，随着分支向底部逐渐变厚。如果角度为0，分支将在其长度上拥有均匀的厚度。一点角度可以增加organic的稳定性。</translation>
    </message>
    <message>
      <source>Branch Diameter with double walls</source>
      <translation>分支直径双层墙</translation>
    </message>
    <message>
      <source>Branches with area larger than the area of a circle of this diameter will be printed with double walls for stability. Set this value to zero for no double walls.</source>
      <translation>该值大于以分支直径得到的圆形面积时，将打印双层墙，以保持稳定性。如不使用双层墙，请将该值设置为0。</translation>
    </message>
    <message>
      <source>Tree support branch distance</source>
      <translation>树状支撑分支距离</translation>
    </message>
    <message>
      <source>This setting determines the distance between neighboring tree support nodes.</source>
      <translation>此设置确定了树状支撑的相邻节点之间的距离。</translation>
    </message>
    <message>
      <source>Tree support brim width</source>
      <translation>树状支撑裙边宽度</translation>
    </message>
    <message>
      <source>Distance from tree branch to the outermost brim line</source>
      <translation>从树状支撑分支到最外层裙边线的距离</translation>
    </message>
    <message>
      <source>Tip Diameter</source>
      <translation>尖端直径</translation>
    </message>
    <message>
      <source>Branch tip diameter for organic supports.</source>
      <translation>Organic支撑分支的尖端直径</translation>
    </message>
    <message>
      <source>Branch Density</source>
      <translation>分支密度</translation>
    </message>
    <message>
      <source>Adjusts the density of the support structure used to generate the tips of the branches. A higher value results in better overhangs but the supports are harder to remove, thus it is recommended to enable top support interfaces instead of a high branch density value if dense interfaces are needed.</source>
      <translation>用于调整分支尖端所生成的支撑结构的密度。值越大悬垂越好，但更难拆支撑，因此，如果需要密集的接触面，建议启用顶部接触面相关参数，而不是较高的分支密度。</translation>
    </message>
    <message>
      <source>Tree support wall loops</source>
      <translation>树状支撑外墙层数</translation>
    </message>
    <message>
      <source>This setting specify the count of walls around tree support</source>
      <translation>树状支撑外墙层数</translation>
    </message>
    <message>
      <source>Tree support with infill</source>
      <translation>树状支撑生成填充</translation>
    </message>
    <message>
      <source>This setting specifies whether to add infill inside large hollows of tree support</source>
      <translation>这个设置决定是否为树状支撑内部的空腔生成填充。</translation>
    </message>
    <message>
      <source>upward compatible machine</source>
      <translation>向上兼容的机器</translation>
    </message>
    <message>
      <source>Use firmware retraction</source>
      <translation>使用固件回抽</translation>
    </message>
    <message>
      <source>This experimental setting uses G10 and G11 commands to have the firmware handle the retraction. This is only supported in recent Marlin.</source>
      <translation>（实验设置）使用G10和G11命令让固件处理回抽。该功能仅支持最近版本的Marlin固件。</translation>
    </message>
    <message>
      <source>Use relative E distances</source>
      <translation>使用相对E距离</translation>
    </message>
    <message>
      <source>Wall distribution count</source>
      <translation>墙分布计数</translation>
    </message>
    <message>
      <source>The number of walls, counted from the center, over which the variation needs to be spread. Lower values mean that the outer walls don't change in width</source>
      <translation>从中心开始计算的墙层数，线宽变化需要分布在这些墙走线上。较低的数值意味着外墙宽度更不易被改变</translation>
    </message>
    <message>
      <source>Walls</source>
      <translation>墙</translation>
    </message>
    <message>
      <source>Classic wall generator produces walls with constant extrusion width and for very thin areas is used gap-fill. Arachne engine produces walls with variable extrusion width</source>
      <translation>经典墙生成器产生的墙走线具有一致的挤出宽度，对狭窄区域使用填缝。Arachne引擎则产生变线宽的墙走线</translation>
    </message>
    <message>
      <source>Arachne</source>
      <translation>Arachne</translation>
    </message>
    <message>
      <source>Classic</source>
      <translation>经典</translation>
    </message>
    <message>
      <source>Order of inner wall/outer wall/infil</source>
      <translation>内墙/外墙/填充的顺序</translation>
    </message>
    <message>
      <source>Print sequence of inner wall, outer wall and infill. </source>
      <translation>内圈墙/外圈墙/填充的打印顺序</translation>
    </message>
    <message>
      <source>infill/inner/outer</source>
      <translation>填充/内墙/外墙</translation>
    </message>
    <message>
      <source>infill/outer/inner</source>
      <translation>填充/外墙/内墙</translation>
    </message>
    <message>
      <source>inner/outer/infill</source>
      <translation>内墙/外墙/填充</translation>
    </message>
    <message>
      <source>inner-outer-inner/infill</source>
      <translation>内墙/外墙/内墙/填充</translation>
    </message>
    <message>
      <source>outer/inner/infill</source>
      <translation>外墙/内墙/填充</translation>
    </message>
    <message>
      <source>Wall loops</source>
      <translation>墙层数</translation>
    </message>
    <message>
      <source>Number of walls of every layer</source>
      <translation>每一层的外墙</translation>
    </message>
    <message>
      <source>Wall transitioning threshold angle</source>
      <translation>墙过渡阈值角度</translation>
    </message>
    <message>
      <source>When to create transitions between even and odd numbers of walls. A wedge shape with an angle greater than this setting will not have transitions and no walls will be printed in the center to fill the remaining space. Reducing this setting reduces the number and length of these center walls, but may leave gaps or overextrude</source>
      <translation>何时在偶数和奇数墙层数之间创建过渡段。角度大于这个阈值的楔形将不创建过渡段，并且不会在楔形中心打印墙走线以填补剩余空间。减小这个数值能减少中心墙走线的数量和长度，但可能会导致间隙或者过挤出</translation>
    </message>
    <message>
      <source>Wall transitioning filter margin</source>
      <translation>墙过渡过滤间距</translation>
    </message>
    <message>
      <source>Prevent transitioning back and forth between one extra wall and one less. This margin extends the range of extrusion widths which follow to [Minimum wall width - margin, 2 * Minimum wall width + margin]. Increasing this margin reduces the number of transitions, which reduces the number of extrusion starts/stops and travel time. However, large extrusion width variation can lead to under- or overextrusion problems. It's expressed as a percentage over nozzle diameter</source>
      <translation>防止特定厚度变化规律的局部在多一层墙和少一层墙之间来回转换。这个参数将挤压宽度的范围扩大到[墙最小宽度-参数值, 2*墙最小宽度+参数值]。增大参数可以减少转换的次数，从而减少挤出开始/停止和空驶的时间。然而，大的挤出宽度变化会导致过挤出或欠挤出的问题。参数值表示为相对于喷嘴直径的百分比</translation>
    </message>
    <message>
      <source>Wall transition length</source>
      <translation>墙过渡长度</translation>
    </message>
    <message>
      <source>When transitioning between different numbers of walls as the part becomes thinner, a certain amount of space is allotted to split or join the wall segments. It's expressed as a percentage over nozzle diameter</source>
      <translation>当零件逐渐变薄导致墙的层数发生变化时，需要在过渡段分配一定的空间来分割和连接墙走线。参数值表示为相对于喷嘴直径的百分比</translation>
    </message>
    <message>
      <source>Wipe on loops</source>
      <translation>闭环擦拭</translation>
    </message>
    <message>
      <source>To minimize the visibility of the seam in a closed loop extrusion, a small inward movement is executed before the extruder leaves the loop.</source>
      <translation>为了最大限度地减少闭环挤出中接缝的可见性，在挤出机离开环之前，会向内执行一个小小的移动。</translation>
    </message>
    <message>
      <source>Wipe speed</source>
      <translation>擦拭速度</translation>
    </message>
    <message>
      <source>The wipe speed is determined by the speed setting specified in this configuration.If the value is expressed as a percentage (e.g. 80%), it will be calculated based on the travel speed setting above.The default value for this parameter is 80%</source>
      <translation>擦拭速度是根据此配置中指定的速度设置确定的。如果该值以百分比形式表示（例如80%），则将根据上方的移动速度设置进行计算。该参数的默认值为80%。</translation>
    </message>
    <message>
      <source>Maximal bridging distance</source>
      <translation>最大桥接距离</translation>
    </message>
    <message>
      <source>Maximal distance between supports on sparse infill sections.</source>
      <translation>稀疏填充剖面上支撑之间的最大距离。</translation>
    </message>
    <message>
      <source>Stabilization cone apex angle</source>
      <translation>稳定锥体顶角</translation>
    </message>
    <message>
      <source>Angle at the apex of the cone that is used to stabilize the wipe tower. Larger angle means wider base.</source>
      <translation>塔锥体顶角的角度，用于稳定擦拭塔。角度越大，底座越宽。</translation>
    </message>
    <message>
      <source>Wipe tower purge lines spacing</source>
      <translation>擦拭塔冲刷线间距</translation>
    </message>
    <message>
      <source>Spacing of purge lines on the wipe tower.</source>
      <translation>擦拭塔上冲刷线的间距</translation>
    </message>
    <message>
      <source>Wipe tower extruder</source>
      <translation>擦拭塔挤出机</translation>
    </message>
    <message>
      <source>The extruder to use when printing perimeter of the wipe tower. Set to 0 to use the one that is available (non-soluble would be preferred).</source>
      <translation>打印擦拭塔周长时使用的挤出机。设置为0将使用唯一的挤出机（尽量使用不可溶的材料）。</translation>
    </message>
    <message>
      <source>No sparse layers (EXPERIMENTAL)</source>
      <translation>无稀疏层（实验）</translation>
    </message>
    <message>
      <source>If enabled, the wipe tower will not be printed on layers with no toolchanges. On layers with a toolchange, extruder will travel downward to print the wipe tower. User is responsible for ensuring there is no collision with the print.</source>
      <translation>如果启用，将不会在没有换色的层打印擦拭塔。存在换色的层时，挤出机将降低高度打印擦拭塔。用户应该确保不会与打印内容发生冲突。</translation>
    </message>
    <message>
      <source>Wipe tower rotation angle</source>
      <translation>擦拭塔旋转角度</translation>
    </message>
    <message>
      <source>Wipe tower rotation angle with respect to x-axis.</source>
      <translation>擦拭塔相对于x轴的旋转角度</translation>
    </message>
    <message>
      <source>This vector saves required volumes to change from/to each tool used on the wipe tower. These values are used to simplify creation of the full purging volumes below.</source>
      <translation>此矢量可保存所需的体积，用于更改每个擦拭塔上工具所使用的from/to体积 。这些值用于简化完全冲刷体积的创建。</translation>
    </message>
    <message>
      <source>X-Y contour compensation</source>
      <translation>X-Y 外轮廓尺寸补偿</translation>
    </message>
    <message>
      <source>Contour of object will be grown or shrunk in XY plane by the configured value. Positive value makes contour bigger. Negative value makes contour smaller. This function is used to adjust size slightly when the object has assembling issue</source>
      <translation>模型外轮廓的尺寸将在X-Y方向收缩或拓展特定值。正值代表扩大。负值代表缩小。这个功能通常在模型有装配问题时微调尺寸</translation>
    </message>
    <message>
      <source>X-Y hole compensation</source>
      <translation>X-Y 孔洞尺寸补偿</translation>
    </message>
    <message>
      <source>Holes of object will be grown or shrunk in XY plane by the configured value. Positive value makes holes bigger. Negative value makes holes smaller. This function is used to adjust size slightly when the object has assembling issue</source>
      <translation>垂直的孔洞的尺寸将在X-Y方向收缩或拓展特定值。正值代表扩大孔洞。负值代表缩小孔洞。这个功能通常在模型有装配问题时微调尺寸</translation>
    </message>
    <message>
      <source>Z offset</source>
      <translation>Z偏移</translation>
    </message>
     <message>
        <source>This value will be added (or subtracted) from all the Z coordinates in the output G-code. It is used to compensate for bad Z endstop position: for example, if your endstop zero actually leaves the nozzle 0.3mm far from the print bed, set this to -0.3 (or fix your endstop).</source>
        <translation>此值将从输出 G-Code 中的所有 Z 坐标中添加(或减去)。它用于补偿损坏的 Z 端限位器置：例如，如果限位器零实际离开喷嘴 0.3mm 远离构建板(打印床)，将其设置为 -0.3（或调整限位器）。</translation>
    </message>
    <message>
      <source>Epoxy Resin Plate</source>
      <translation>环氧树脂面板</translation>
    </message>
  </context>
</TS>
