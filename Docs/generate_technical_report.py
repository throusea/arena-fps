from __future__ import annotations

import os
from pathlib import Path

from reportlab.graphics.shapes import Drawing, Line, Polygon, Rect, String
from reportlab.lib import colors
from reportlab.lib.colors import HexColor
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate,
    Flowable,
    Frame,
    KeepTogether,
    NextPageTemplate,
    PageBreak,
    PageTemplate,
    Paragraph,
    Spacer,
    Table,
    TableStyle,
)


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = Path(os.environ.get(
    "ARENA_REPORT_OUTPUT",
    ROOT / "output" / "pdf" / "Arena_FPS_技术说明文档.pdf",
))

NAVY = HexColor("#15243A")
BLUE = HexColor("#2F6FED")
CYAN = HexColor("#18A0AE")
ORANGE = HexColor("#F18F3B")
INK = HexColor("#172033")
MUTED = HexColor("#5D6A7E")
PALE = HexColor("#EEF4FF")
PALE_CYAN = HexColor("#EAF8F8")
PALE_ORANGE = HexColor("#FFF3E8")
LINE = HexColor("#D9E1EC")


def register_fonts() -> None:
    pdfmetrics.registerFont(TTFont("CN", r"C:\Windows\Fonts\msyh.ttc", subfontIndex=0))
    pdfmetrics.registerFont(TTFont("CN-Bold", r"C:\Windows\Fonts\msyhbd.ttc", subfontIndex=0))


def P(text: str, style) -> Paragraph:
    return Paragraph(text, style)


class SectionRule(Flowable):
    def __init__(self, width: float):
        super().__init__()
        self.width = width
        self.height = 4 * mm

    def draw(self):
        self.canv.setStrokeColor(BLUE)
        self.canv.setLineWidth(2)
        self.canv.line(0, 2.5 * mm, 17 * mm, 2.5 * mm)
        self.canv.setStrokeColor(LINE)
        self.canv.setLineWidth(0.6)
        self.canv.line(20 * mm, 2.5 * mm, self.width, 2.5 * mm)


def box(d: Drawing, x, y, w, h, title, subtitle, fill, stroke=LINE):
    d.add(Rect(x, y, w, h, 7, 7, fillColor=fill, strokeColor=stroke, strokeWidth=1))
    d.add(String(x + w / 2, y + h - 15, title, textAnchor="middle", fontName="CN-Bold", fontSize=9, fillColor=INK))
    if subtitle:
        lines = subtitle.split("\n")
        for i, item in enumerate(lines):
            d.add(String(x + w / 2, y + h - 31 - i * 12, item, textAnchor="middle", fontName="CN", fontSize=7.2, fillColor=MUTED))


def arrow(d: Drawing, x1, y1, x2, y2, color=BLUE):
    d.add(Line(x1, y1, x2, y2, strokeColor=color, strokeWidth=1.4))
    import math
    angle = math.atan2(y2 - y1, x2 - x1)
    size = 5
    p1 = (x2, y2)
    p2 = (x2 - size * math.cos(angle - 0.55), y2 - size * math.sin(angle - 0.55))
    p3 = (x2 - size * math.cos(angle + 0.55), y2 - size * math.sin(angle + 0.55))
    d.add(Polygon([*p1, *p2, *p3], fillColor=color, strokeColor=color))


def architecture_diagram() -> Drawing:
    d = Drawing(170 * mm, 78 * mm)
    box(d, 4 * mm, 52 * mm, 46 * mm, 20 * mm, "Dedicated Server", "GameMode / AI / Damage\n本地测试的权威服务器", PALE_ORANGE, ORANGE)
    box(d, 62 * mm, 52 * mm, 46 * mm, 20 * mm, "复制状态", "GameState / PlayerState\nHealth / Weapon State", PALE, BLUE)
    box(d, 120 * mm, 52 * mm, 46 * mm, 20 * mm, "Game Client", "输入请求 / 本地表现\nHUD / Widget Blueprint", PALE_CYAN, CYAN)
    arrow(d, 50 * mm, 62 * mm, 62 * mm, 62 * mm, ORANGE)
    arrow(d, 108 * mm, 62 * mm, 120 * mm, 62 * mm, BLUE)
    arrow(d, 120 * mm, 56 * mm, 108 * mm, 56 * mm, CYAN)
    box(d, 14 * mm, 10 * mm, 40 * mm, 24 * mm, "Server RPC", "开火 / 切枪\n客户端只发意图", PALE, BLUE)
    box(d, 65 * mm, 10 * mm, 40 * mm, 24 * mm, "RepNotify", "生命 / 弹药 / 分数\n胜者 / 当前武器", PALE_CYAN, CYAN)
    box(d, 116 * mm, 10 * mm, 40 * mm, 24 * mm, "Multicast / Client", "公共特效可丢失\n命中确认可靠且私有", PALE_ORANGE, ORANGE)
    arrow(d, 34 * mm, 34 * mm, 30 * mm, 52 * mm, BLUE)
    arrow(d, 85 * mm, 52 * mm, 85 * mm, 34 * mm, CYAN)
    arrow(d, 140 * mm, 34 * mm, 142 * mm, 52 * mm, ORANGE)
    return d


def weapon_diagram() -> Drawing:
    d = Drawing(170 * mm, 77 * mm)
    box(d, 55 * mm, 52 * mm, 60 * mm, 20 * mm, "ANetWeaponBase", "弹药 / 装填 / 散布 / 表现事件\n服务器权威开火入口", PALE, BLUE)
    box(d, 8 * mm, 14 * mm, 48 * mm, 22 * mm, "ANetRifle", "Hitscan LineTrace\n步枪 + 手枪配置", PALE_CYAN, CYAN)
    box(d, 64 * mm, 14 * mm, 48 * mm, 22 * mm, "ANetProjectileWeapon", "服务器生成 Projectile\n榴弹发射器", PALE_ORANGE, ORANGE)
    box(d, 120 * mm, 14 * mm, 42 * mm, 22 * mm, "INetWeaponHolder", "角色库存 / 挂接\nHUD 通信接口", PALE, BLUE)
    arrow(d, 75 * mm, 52 * mm, 34 * mm, 36 * mm, CYAN)
    arrow(d, 92 * mm, 52 * mm, 88 * mm, 36 * mm, ORANGE)
    arrow(d, 112 * mm, 57 * mm, 141 * mm, 36 * mm, BLUE)
    return d


def hit_flow_diagram() -> Drawing:
    d = Drawing(170 * mm, 104 * mm)
    nodes = [
        ("1  输入", "DoStartFiring"),
        ("2  服务器 RPC", "ServerStartFiring"),
        ("3  权威判定", "FireOnServer + Damage"),
        ("4  私有回执", "ClientConfirmHit (Reliable)"),
        ("5  角色路由", "ReportWeaponHitConfirmed"),
        ("6  C++ 委托", "OnWeaponHitConfirmed"),
        ("7  HUD 桥接", "HandleWeaponHit"),
        ("8  蓝图表现", "BP_OnHitConfirmed"),
    ]
    x_positions = [5, 88]
    y_positions = [79, 53, 27, 1]
    coords = []
    for i, (title, subtitle) in enumerate(nodes):
        col = i % 2
        row = i // 2
        x = x_positions[col] * mm
        y = y_positions[row] * mm
        fill = PALE_ORANGE if i in (2, 3) else (PALE_CYAN if i >= 6 else PALE)
        stroke = ORANGE if i in (2, 3) else (CYAN if i >= 6 else BLUE)
        box(d, x, y, 76 * mm, 20 * mm, title, subtitle, fill, stroke)
        coords.append((x, y))
    for i in range(len(coords) - 1):
        x, y = coords[i]
        nx, ny = coords[i + 1]
        if i % 2 == 0:
            arrow(d, x + 76 * mm, y + 10 * mm, nx, ny + 10 * mm)
        else:
            arrow(d, x + 38 * mm, y, nx + 38 * mm, ny + 20 * mm)
    return d


def score_diagram() -> Drawing:
    d = Drawing(170 * mm, 65 * mm)
    labels = [
        ("服务器死亡判定", "NPC / Player", PALE_ORANGE, ORANGE),
        ("GameMode", "+1 / +3 分\n检查 10 分胜利", PALE, BLUE),
        ("PlayerState", "复制分数\n跨重生保留", PALE_CYAN, CYAN),
        ("GameState", "PlayerArray 排序\n复制胜者", PALE, BLUE),
        ("HUD", "实时榜单\n胜利提示", PALE_CYAN, CYAN),
    ]
    for i, (title, subtitle, fill, stroke) in enumerate(labels):
        x = (2 + i * 34) * mm
        box(d, x, 20 * mm, 30 * mm, 28 * mm, title, subtitle, fill, stroke)
        if i < len(labels) - 1:
            arrow(d, x + 30 * mm, 34 * mm, x + 34 * mm, 34 * mm, stroke)
    d.add(String(85 * mm, 8 * mm, "死亡玩家: 5 秒 Timer -> 销毁旧 Pawn -> RestartPlayer -> 随机 PlayerStart", textAnchor="middle", fontName="CN", fontSize=8, fillColor=MUTED))
    return d


def build_styles():
    styles = getSampleStyleSheet()
    return {
        "title": ParagraphStyle("title", fontName="CN-Bold", fontSize=28, leading=38, textColor=colors.white, alignment=TA_LEFT),
        "subtitle": ParagraphStyle("subtitle", fontName="CN", fontSize=12, leading=20, textColor=HexColor("#D9E6FF")),
        "cover_label": ParagraphStyle("cover_label", fontName="CN", fontSize=8, leading=12, textColor=HexColor("#AFC1D9")),
        "cover_value": ParagraphStyle("cover_value", fontName="CN", fontSize=9.2, leading=14, textColor=colors.white),
        "h1": ParagraphStyle("h1", fontName="CN-Bold", fontSize=18, leading=25, textColor=NAVY, spaceBefore=2 * mm, spaceAfter=1 * mm),
        "h2": ParagraphStyle("h2", fontName="CN-Bold", fontSize=12.5, leading=18, textColor=BLUE, spaceBefore=3 * mm, spaceAfter=1.5 * mm),
        "body": ParagraphStyle("body", fontName="CN", fontSize=9.4, leading=16, textColor=INK, spaceAfter=2.2 * mm, wordWrap="CJK", allowWidows=0, allowOrphans=0),
        "small": ParagraphStyle("small", fontName="CN", fontSize=7.8, leading=12, textColor=MUTED, wordWrap="CJK"),
        "bullet": ParagraphStyle("bullet", fontName="CN", fontSize=9.1, leading=15, leftIndent=5 * mm, firstLineIndent=-3.5 * mm, bulletIndent=0, textColor=INK, spaceAfter=1.2 * mm, wordWrap="CJK"),
        "callout": ParagraphStyle("callout", fontName="CN", fontSize=9.1, leading=15, leftIndent=4 * mm, rightIndent=4 * mm, borderColor=BLUE, borderWidth=0.8, borderPadding=4 * mm, backColor=PALE, textColor=INK, wordWrap="CJK"),
        "table": ParagraphStyle("table", fontName="CN", fontSize=8, leading=12, textColor=INK, wordWrap="CJK"),
        "table_head": ParagraphStyle("table_head", fontName="CN-Bold", fontSize=8, leading=12, textColor=colors.white, alignment=TA_CENTER),
        "footer": ParagraphStyle("footer", fontName="CN", fontSize=7, textColor=MUTED, alignment=TA_CENTER),
    }


def page_decor(canvas, doc):
    canvas.saveState()
    page = canvas.getPageNumber()
    if page > 1:
        canvas.setStrokeColor(LINE)
        canvas.setLineWidth(0.5)
        canvas.line(20 * mm, 16 * mm, 190 * mm, 16 * mm)
        canvas.setFont("CN", 7)
        canvas.setFillColor(MUTED)
        canvas.drawString(20 * mm, 10 * mm, "Arena FPS 技术说明文档")
        canvas.drawRightString(190 * mm, 10 * mm, f"{page - 1:02d}")
    canvas.restoreState()


def cover(canvas, doc):
    canvas.saveState()
    w, h = A4
    canvas.setFillColor(NAVY)
    canvas.rect(0, 0, w, h, fill=1, stroke=0)
    canvas.setFillColor(BLUE)
    canvas.circle(178 * mm, 267 * mm, 56 * mm, fill=1, stroke=0)
    canvas.setFillColor(CYAN)
    canvas.circle(25 * mm, 22 * mm, 45 * mm, fill=1, stroke=0)
    canvas.setFillColor(ORANGE)
    canvas.rect(20 * mm, 232 * mm, 18 * mm, 3 * mm, fill=1, stroke=0)
    canvas.restoreState()


def heading(story, title, styles):
    story.append(P(title, styles["h1"]))
    story.append(SectionRule(170 * mm))


def bullet(story, text, styles):
    story.append(P(f"<bullet>•</bullet>{text}", styles["bullet"]))


def build_pdf() -> None:
    register_fonts()
    styles = build_styles()
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    doc = BaseDocTemplate(
        str(OUTPUT), pagesize=A4, leftMargin=20 * mm, rightMargin=20 * mm,
        topMargin=18 * mm, bottomMargin=21 * mm, title="Arena FPS 技术说明文档",
        author="Arena FPS Project", subject="UE5 Multiplayer FPS Course Technical Report",
    )
    frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="main")
    doc.addPageTemplates([
        PageTemplate(id="cover", frames=[frame], onPage=cover),
        PageTemplate(id="content", frames=[frame], onPage=page_decor),
    ])
    story = []

    story += [Spacer(1, 52 * mm), P("ARENA FPS", styles["title"]), P("多人第一人称竞技场 Demo", styles["title"]), Spacer(1, 6 * mm), P("UE5 网络同步与游戏系统技术说明", styles["subtitle"]), Spacer(1, 60 * mm)]
    cover_table = Table([
        [P("引擎", styles["cover_label"]), P("Unreal Engine 5.7", styles["cover_value"])],
        [P("网络模式", styles["cover_label"]), P("本地 Dedicated Server / 服务器权威架构", styles["cover_value"])],
        [P("项目仓库", styles["cover_label"]), P('<link href="https://github.com/throusea/arena-fps" color="#D9E6FF">github.com/throusea/arena-fps</link>', styles["cover_value"])],
        [P("文档日期", styles["cover_label"]), P("2026-06-21", styles["cover_value"])],
    ], colWidths=[28 * mm, 112 * mm])
    cover_table.setStyle(TableStyle([("BACKGROUND", (0,0), (-1,-1), HexColor("#203451")), ("BOX", (0,0), (-1,-1), 0.6, HexColor("#456181")), ("INNERGRID", (0,0), (-1,-1), 0.4, HexColor("#456181")), ("VALIGN", (0,0), (-1,-1), "MIDDLE"), ("LEFTPADDING", (0,0), (-1,-1), 4 * mm), ("RIGHTPADDING", (0,0), (-1,-1), 4 * mm), ("TOPPADDING", (0,0), (-1,-1), 2.5 * mm), ("BOTTOMPADDING", (0,0), (-1,-1), 2.5 * mm)]))
    story.append(cover_table)
    story.append(NextPageTemplate("content"))
    story.append(PageBreak())

    heading(story, "01  项目概述与玩法", styles)
    story.append(P("Arena FPS 基于 UE5 官方 First Person 模板开发，是一款多人第一人称竞技场 Demo。项目仅在本地使用 Dedicated Server 进行多人测试，整体采用服务器权威架构。玩家在灰盒竞技场中移动、瞄准、射击、拾取和切换武器，同时对抗其他玩家与服务器控制的敌人。", styles["body"]))
    gameplay = [
        [P("玩法要素", styles["table_head"]), P("当前实现", styles["table_head"])],
        [P("战斗", styles["table"]), P("步枪、手枪、榴弹发射器；Hitscan 与复制投射物；命中、散布、后坐力、装填和特效反馈", styles["table"])],
        [P("敌人", styles["table"]), P("服务器 AI Perception + Behavior Tree；追击最近存活玩家，近身攻击，可受击、死亡和按波次生成", styles["table"])],
        [P("计分", styles["table"]), P("击败敌人 +1 分，击败其他玩家 +3 分，自杀不计分；默认 10 分获胜", styles["table"])],
        [P("重生", styles["table"]), P("玩家死亡后进入死亡镜头，5 秒后在随机 PlayerStart 重生；产生胜者后停止重生", styles["table"])],
        [P("多人 HUD", styles["table"]), P("生命、弹药、装填、武器名、准星、命中确认、个人分数、实时榜单和胜利提示", styles["table"])],
    ]
    t = Table(gameplay, colWidths=[28 * mm, 142 * mm], repeatRows=1)
    t.setStyle(TableStyle([("BACKGROUND", (0,0), (-1,0), NAVY), ("GRID", (0,0), (-1,-1), 0.5, LINE), ("VALIGN", (0,0), (-1,-1), "TOP"), ("ROWBACKGROUNDS", (0,1), (-1,-1), [colors.white, HexColor("#F8FAFD")]), ("LEFTPADDING", (0,0), (-1,-1), 3 * mm), ("RIGHTPADDING", (0,0), (-1,-1), 3 * mm), ("TOPPADDING", (0,0), (-1,-1), 2.6 * mm), ("BOTTOMPADDING", (0,0), (-1,-1), 2.6 * mm)]))
    story += [t, Spacer(1, 5 * mm), P("<b>课程要求覆盖：</b>敌人能够移动和攻击，玩家能够击败敌人；实现得分与胜利；支持多人网络对战。", styles["callout"]), Spacer(1, 4 * mm)]
    heading(story, "02  基于网络同步的游戏架构", styles)
    story.append(P("项目采用 Client-Server 服务器权威模型，并仅在本地通过 Dedicated Server 与多个 Client 进程进行测试。Dedicated Server 不承载本地玩家，只负责权威规则和状态；Client 只提交操作意图，不直接决定伤害、分数、死亡、重生和胜负。", styles["body"]))
    story.append(architecture_diagram())
    story.append(PageBreak())

    heading(story, "03  网络同步思想", styles)
    for text in [
        "<b>Reliable Server RPC：</b>开火、停止开火、切换武器等关键输入只提交意图，服务器验证角色状态后执行。",
        "<b>属性复制 + OnRep：</b>生命、当前武器、弹药、装填、散布、分数和胜者属于持久状态，保证迟加入客户端也能恢复当前值。",
        "<b>Unreliable NetMulticast：</b>枪口火焰、普通命中特效和攻击表现频率较高，偶尔丢失不会改变游戏结果。",
        "<b>Reliable owner-only Client RPC：</b>服务器确认实际造成伤害后，只向射击者返回命中确认，避免客户端猜测与无关广播。",
        "<b>Authority 边界：</b>AI、Trace、Projectile 生成、伤害、敌人生成、计分、胜利和重生全部由服务器执行。",
    ]:
        bullet(story, text, styles)
    story.append(Spacer(1, 3 * mm))
    roles = [
        [P("类", styles["table_head"]), P("网络职责", styles["table_head"])],
        [P("ANetGameMode", styles["table"]), P("仅服务器存在；差异化计分、胜利检查、重生调度与 PlayerStart 选择", styles["table"])],
        [P("ANetGameState", styles["table"]), P("复制目标分数、胜者；从 PlayerArray 生成全员排序榜单", styles["table"])],
        [P("ANetPlayerStateBase", styles["table"]), P("复制玩家名称与 KillScore，跨 Pawn 重生保留比赛成绩", styles["table"])],
        [P("ANetCharacter / ANetNPC", styles["table"]), P("复制移动、生命、死亡与当前武器；将权威结果路由给表现层", styles["table"])],
        [P("ANetWeaponBase", styles["table"]), P("服务器开火与伤害；复制弹药、装填、散布；分发公共特效和私有命中确认", styles["table"])],
        [P("UNetHUDWidget", styles["table"]), P("仅本地创建；订阅复制数据源的动态委托，调用 Blueprint 表现事件", styles["table"])],
    ]
    t = Table(roles, colWidths=[45 * mm, 125 * mm], repeatRows=1)
    t.setStyle(TableStyle([("BACKGROUND", (0,0), (-1,0), NAVY), ("GRID", (0,0), (-1,-1), 0.5, LINE), ("VALIGN", (0,0), (-1,-1), "TOP"), ("ROWBACKGROUNDS", (0,1), (-1,-1), [colors.white, HexColor("#F8FAFD")]), ("LEFTPADDING", (0,0), (-1,-1), 3 * mm), ("RIGHTPADDING", (0,0), (-1,-1), 3 * mm), ("TOPPADDING", (0,0), (-1,-1), 2.4 * mm), ("BOTTOMPADDING", (0,0), (-1,-1), 2.4 * mm)]))
    story += [t, Spacer(1, 5 * mm)]
    heading(story, "04  敌人与战斗流程", styles)
    story.append(P("服务器上的 <b>ANetAIController</b> 使用 AI Perception 维护可见玩家，选择最近的存活目标写入 Blackboard，Behavior Tree 负责追击和攻击决策。<b>ANetNPC::TryAttack</b> 仍会检查权限、距离、冷却和攻击前摇，最终由服务器调用 ApplyDamage。", styles["body"]))
    story.append(P("伤害统一进入 <b>UNetHealthComponent</b>。服务器扣减 Health 并复制 Health 与 bIsDead；NPC 死亡后停止行为树和移动、通知 GameMode 计分并延迟销毁。敌人波次和 SpawnPoint 同样只由服务器驱动，客户端接收已复制的 NPC Actor 与移动结果。", styles["body"]))
    story.append(PageBreak())

    heading(story, "05  武器系统设计", styles)
    story.append(weapon_diagram())
    story.append(P("<b>ANetWeaponBase</b> 集中实现弹药、自动装填、射速、连续射击散布、移动/滞空惩罚、散布恢复、第一/第三人称 Mesh、动画、后坐力、Camera Shake、音效、Niagara 以及 C++/Blueprint 表现事件。武器 Actor 启用复制并使用 Owner Relevancy。", styles["body"]))
    for text in [
        "<b>Hitscan 分支：</b>ANetRifle 在服务器从玩家视点计算带散布方向，使用 WeaponTrace 执行 LineTrace 并调用 ApplyPointDamage。步枪和手枪通过 Blueprint 参数形成不同手感。",
        "<b>Projectile 分支：</b>ANetProjectileWeapon 在服务器生成可复制移动的 ANetProjectile；碰撞后执行单体或范围伤害，榴弹发射器使用这一流程。",
        "<b>持有者接口：</b>INetWeaponHolder 解耦武器与角色，负责 Mesh 挂接、动画、后坐力、HUD 更新与命中回报。角色维护武器库存并复制当前武器。",
        "<b>拾取系统：</b>ANetWeaponPickup 用 DataTable 配置 Mesh 与武器类，服务器处理重叠授予和定时刷新，防止多个客户端同时领取。",
    ]:
        bullet(story, text, styles)
    story.append(Spacer(1, 3 * mm))
    story.append(P("<b>C++ 与 Blueprint 分工：</b>C++ 保证网络规则和数据真实性；Blueprint 配置具体武器参数、资产、动画、特效与 Widget 视觉，兼顾可靠性和快速迭代。", styles["callout"]))
    story.append(PageBreak())

    heading(story, "06  BP_OnHitConfirmed 通信设计链路", styles)
    story.append(P("命中反馈不采用“客户端准星碰到目标就显示”的猜测逻辑，而是等待服务器确认实际伤害。完整事件链如下。", styles["body"]))
    story.append(hit_flow_diagram())
    story.append(Spacer(1, 2 * mm))
    story.append(P("服务器在 <b>FNetWeaponImpactResult::bDamagedActor == true</b> 时通过 ClientConfirmHit 回执。Owner Client 上的武器调用 Character 的 ReportWeaponHitConfirmed，Character 广播 OnWeaponHitConfirmed；UNetHUDWidget 订阅后调用 BP_OnHitConfirmed，由 WBP_NetHUD 播放准星变色或 Hit Marker 动画。", styles["body"]))
    story.append(P("这条链路将职责拆成三段：<b>服务器负责真实性，C++ 委托负责数据路由，Widget Blueprint 负责视觉表现</b>。Reliable owner-only 回执既避免误报，也不会把个人命中反馈广播给所有玩家。", styles["callout"]))
    story.append(PageBreak())

    heading(story, "07  计分、胜利、重生与多人榜单", styles)
    story.append(score_diagram())
    story.append(P("服务器判定死亡后，GameMode 根据目标类型发放分数：敌人 1 分，其他玩家 3 分，自杀不计分。分数写入 PlayerState，因此旧 Pawn 销毁和重新生成不会丢失成绩。每次加分后检查默认 10 分目标，胜者写入 GameState 并复制到所有客户端。", styles["body"]))
    story.append(P("GameState 从 UE 的 PlayerArray 读取所有 ANetPlayerStateBase，按分数降序、名称升序生成 FNetScoreboardEntry。玩家加入、离开、改名或分数变化都会触发 OnScoreboardChanged，两端 HUD 实时重建一致的榜单。", styles["body"]))
    story.append(P("玩家死亡后由服务器启动 5 秒 Timer，随后销毁旧 Pawn 并调用 RestartPlayer。GameMode 每次重生都重新执行随机且避让占用的 PlayerStart 选择；产生胜者后不再安排新重生。", styles["body"]))
    story.append(P("本节覆盖死亡重生、差异化计分与多人实时榜单等核心游戏流程。", styles["callout"]))
    story.append(Spacer(1, 7 * mm))
    heading(story, "08  实现索引与交付信息", styles)
    index_rows = [
        [P("系统", styles["table_head"]), P("核心位置", styles["table_head"])],
        [P("角色 / 规则", styles["table"]), P("Source/Arena/Variant_Network/NetCharacter.*, NetGameMode.*, NetGameState.*, NetPlayerStateBase.*", styles["table"])],
        [P("生命 / AI", styles["table"]), P("Components/NetHealthComponent.*, NetNPC.*, AI/NetAIController.*, Spawning/*", styles["table"])],
        [P("武器", styles["table"]), P("Weapons/NetRifle.*, NetProjectileWeapon.*, NetProjectile.*, NetWeaponPickup.*", styles["table"])],
        [P("HUD", styles["table"]), P("UI/NetHUDWidget.* 与 Content/Variant_Network/Blueprints/Widgets/", styles["table"])],
        [P("关卡 / 蓝图", styles["table"]), P("Content/Variant_Network/", styles["table"])],
    ]
    t = Table(index_rows, colWidths=[35 * mm, 135 * mm], repeatRows=1)
    t.setStyle(TableStyle([("BACKGROUND", (0,0), (-1,0), NAVY), ("GRID", (0,0), (-1,-1), 0.5, LINE), ("VALIGN", (0,0), (-1,-1), "TOP"), ("ROWBACKGROUNDS", (0,1), (-1,-1), [colors.white, HexColor("#F8FAFD")]), ("LEFTPADDING", (0,0), (-1,-1), 3 * mm), ("RIGHTPADDING", (0,0), (-1,-1), 3 * mm), ("TOPPADDING", (0,0), (-1,-1), 2.4 * mm), ("BOTTOMPADDING", (0,0), (-1,-1), 2.4 * mm)]))
    story += [t, Spacer(1, 6 * mm), P('<b>GitHub 仓库：</b><link href="https://github.com/throusea/arena-fps" color="#2F6FED">https://github.com/throusea/arena-fps</link>', styles["callout"]), PageBreak()]
    heading(story, "09  Agent（Codex）与 Skill 使用说明", styles)
    story.append(P("项目开发过程中使用 Codex 作为辅助开发 Agent。Codex 参与提供部分实现思路、拆解技术方案、编写与重构部分代码、分析网络通信链路，并协助生成和排版本技术文档。功能取舍、工程整合与最终运行结果仍由开发者确认。", styles["body"]))
    skill_rows = [
        [P("Skill", styles["table_head"]), P("用途", styles["table_head"])],
        [P("read-project-report", styles["table"]), P("扫描工程结构、配置、源码、资产边界和 Git 状态", styles["table"])],
        [P("github", styles["table"]), P("读取课程需求、功能需求及相关验收标准", styles["table"])],
        [P("documentation / pdf", styles["table"]), P("组织技术说明、生成 PDF，并逐页检查字体、图表、链接和可读性", styles["table"])],
        [P("ue-cpp-foundations", styles["table"]), P("规范 UObject 反射宏、UE 容器、委托与对象生命周期", styles["table"])],
        [P("ue-gameplay-abilities", styles["table"]), P("为 GAS、属性、效果与 Gameplay Tag 等扩展方向提供参考", styles["table"])],
        [P("ue-module-build-system", styles["table"]), P("辅助处理 Build.cs、模块依赖、IWYU 与编译链接问题", styles["table"])],
    ]
    t = Table(skill_rows, colWidths=[48 * mm, 122 * mm], repeatRows=1)
    t.setStyle(TableStyle([("BACKGROUND", (0,0), (-1,0), NAVY), ("GRID", (0,0), (-1,-1), 0.5, LINE), ("VALIGN", (0,0), (-1,-1), "TOP"), ("ROWBACKGROUNDS", (0,1), (-1,-1), [colors.white, HexColor("#F8FAFD")]), ("LEFTPADDING", (0,0), (-1,-1), 3 * mm), ("RIGHTPADDING", (0,0), (-1,-1), 3 * mm), ("TOPPADDING", (0,0), (-1,-1), 2.2 * mm), ("BOTTOMPADDING", (0,0), (-1,-1), 2.2 * mm)]))
    story.append(t)

    doc.build(story)
    print(OUTPUT)


if __name__ == "__main__":
    build_pdf()
