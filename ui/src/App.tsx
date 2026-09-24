import { For, Show, createEffect, createMemo, createSignal } from 'solid-js'
import './app.css'
import { Index } from 'solid-js'

type Color = [number, number, number, number]

type AttackWarning = {
    id: number
    direction: number
    xPercent: number
    yPercent: number
    resolutionScale: number
    distanceScale: number
}

type NpcUiConfig = {
    enabled: boolean
    mergeWithPrevious: boolean[]
    mergeDirection8With1: boolean
    scalePercent: number
    opacityPercent: number
    offsetXPixels: number
    offsetYPixels: number
    rotationDegrees: number
    diameterPixels: number
    innerDiameterPercent: number
    segmentGapPixels: number
    activeColor: Color
    inactiveColor: Color
    borderColor: Color
    sharedSegmentImage: string
    segmentImages: string[]
}

type UiConfig = {
    enabled: boolean
    editMode: boolean
    mergeWithPrevious: boolean[]
    mergeDirection8With1: boolean
    showCenter: boolean
    moveCenterWithDirection: boolean
    centerInThirdPerson: boolean
    scaleWithResolution: boolean
    positionXPercent: number
    positionYPercent: number
    attachOffsetXPixels: number
    attachOffsetYPixels: number
    scalePercent: number
    diameterPixels: number
    innerDiameterPercent: number
    centerPieceSizePercent: number
    segmentGapPixels: number
    rotationDegrees: number
    opacityPercent: number
    centerMovementPixels: number
    centerMovementDurationMs: number
    activeColor: Color
    inactiveColor: Color
    borderColor: Color
    centerColor: Color
    sharedSegmentImage: string
    segmentImages: string[]
    centerImage: string
    npc: NpcUiConfig
}

const defaultNpcConfig = (): NpcUiConfig => ({
    enabled: true,
    mergeWithPrevious: Array.from({ length: 8 }, () => false),
    mergeDirection8With1: false,
    scalePercent: 52,
    opacityPercent: 100,
    offsetXPixels: 0,
    offsetYPixels: 0,
    rotationDegrees: 0,
    diameterPixels: 260,
    innerDiameterPercent: 36,
    segmentGapPixels: 4,
    activeColor: [0.95, 0.72, 0.22, 1],
    inactiveColor: [0.08, 0.09, 0.1, 0.72],
    borderColor: [0.8, 0.8, 0.76, 0.9],
    sharedSegmentImage: '',
    segmentImages: Array.from({ length: 8 }, () => ''),
})

const defaultConfig = (): UiConfig => ({
    enabled: true,
    editMode: false,
    mergeWithPrevious: Array.from({ length: 8 }, () => false),
    mergeDirection8With1: false,
    showCenter: true,
    moveCenterWithDirection: false,
    centerInThirdPerson: false,
    scaleWithResolution: true,
    positionXPercent: 50,
    positionYPercent: 50,
    attachOffsetXPixels: 0,
    attachOffsetYPixels: 0,
    scalePercent: 100,
    diameterPixels: 260,
    innerDiameterPercent: 36,
    centerPieceSizePercent: 100,
    segmentGapPixels: 4,
    rotationDegrees: 0,
    opacityPercent: 100,
    centerMovementPixels: 20,
    centerMovementDurationMs: 100,
    activeColor: [0.95, 0.72, 0.22, 1],
    inactiveColor: [0.08, 0.09, 0.1, 0.72],
    borderColor: [0.8, 0.8, 0.76, 0.9],
    centerColor: [0.04, 0.04, 0.05, 0.86],
    sharedSegmentImage: '',
    segmentImages: Array.from({ length: 8 }, () => ''),
    centerImage: '',
    npc: defaultNpcConfig(),
})

const clamp = (value: number, minimum: number, maximum: number) =>
    Math.min(maximum, Math.max(minimum, Number.isFinite(value) ? value : minimum))

const parseColor = (value: unknown, fallback: Color): Color => {
    if (!Array.isArray(value) || value.length < 3) return fallback
    return [
        clamp(Number(value[0]), 0, 1),
        clamp(Number(value[1]), 0, 1),
        clamp(Number(value[2]), 0, 1),
        clamp(Number(value[3] ?? 1), 0, 1),
    ]
}

const safeAssetPath = (value: unknown) => {
    if (typeof value !== 'string') return ''
    const path = value.replaceAll('\\', '/').trim()
    if (!path || path.includes('..') || path.includes(':') || path.startsWith('/') || path.startsWith('//')) return ''
    return path.startsWith('./') ? path : `./${path}`
}

const parseConfig = (payload: string): UiConfig | null => {
    try {
        const value = JSON.parse(payload) as Partial<UiConfig>
        const fallback = defaultConfig()
        const npc: Partial<NpcUiConfig> = value.npc ?? {}
        const npcFallback = fallback.npc
        return {
            enabled: value.enabled !== false,
            editMode: value.editMode === true,
            mergeWithPrevious: Array.from({ length: 8 }, (_, index) =>
                index > 0 && Array.isArray(value.mergeWithPrevious) && value.mergeWithPrevious[index] === true),
            mergeDirection8With1: value.mergeDirection8With1 === true,
            showCenter: value.showCenter !== false,
            moveCenterWithDirection: value.moveCenterWithDirection === true,
            centerInThirdPerson: value.centerInThirdPerson === true,
            scaleWithResolution: value.scaleWithResolution !== false,
            positionXPercent: clamp(Number(value.positionXPercent ?? fallback.positionXPercent), 0, 100),
            positionYPercent: clamp(Number(value.positionYPercent ?? fallback.positionYPercent), 0, 100),
            attachOffsetXPixels: clamp(Number(value.attachOffsetXPixels ?? 0), -1000, 1000),
            attachOffsetYPixels: clamp(Number(value.attachOffsetYPixels ?? 0), -1000, 1000),
            scalePercent: clamp(Number(value.scalePercent ?? fallback.scalePercent), 25, 300),
            diameterPixels: clamp(Number(value.diameterPixels ?? fallback.diameterPixels), 96, 800),
            innerDiameterPercent: clamp(Number(value.innerDiameterPercent ?? fallback.innerDiameterPercent), 10, 80),
            centerPieceSizePercent: clamp(Number(value.centerPieceSizePercent ?? fallback.centerPieceSizePercent), 10, 200),
            segmentGapPixels: clamp(Number(value.segmentGapPixels ?? fallback.segmentGapPixels), 0, 30),
            rotationDegrees: clamp(Number(value.rotationDegrees ?? fallback.rotationDegrees), -180, 180),
            opacityPercent: clamp(Number(value.opacityPercent ?? fallback.opacityPercent), 10, 100),
            centerMovementPixels: clamp(Number(value.centerMovementPixels ?? fallback.centerMovementPixels), 0, 400),
            centerMovementDurationMs: clamp(Number(value.centerMovementDurationMs ?? fallback.centerMovementDurationMs), 0, 1000),
            activeColor: parseColor(value.activeColor, fallback.activeColor),
            inactiveColor: parseColor(value.inactiveColor, fallback.inactiveColor),
            borderColor: parseColor(value.borderColor, fallback.borderColor),
            centerColor: parseColor(value.centerColor, fallback.centerColor),
            sharedSegmentImage: safeAssetPath(value.sharedSegmentImage),
            segmentImages: Array.from({ length: 8 }, (_, index) =>
                safeAssetPath(Array.isArray(value.segmentImages) ? value.segmentImages[index] : '')),
            centerImage: safeAssetPath(value.centerImage),
            npc: {
                enabled: npc.enabled !== false,
                mergeWithPrevious: Array.from({ length: 8 }, (_, index) =>
                    index > 0 && Array.isArray(npc.mergeWithPrevious) && npc.mergeWithPrevious[index] === true),
                mergeDirection8With1: npc.mergeDirection8With1 === true,
                scalePercent: clamp(Number(npc.scalePercent ?? npcFallback.scalePercent), 20, 200),
                opacityPercent: clamp(Number(npc.opacityPercent ?? npcFallback.opacityPercent), 10, 100),
                offsetXPixels: clamp(Number(npc.offsetXPixels ?? npcFallback.offsetXPixels), -1000, 1000),
                offsetYPixels: clamp(Number(npc.offsetYPixels ?? npcFallback.offsetYPixels), -1000, 1000),
                rotationDegrees: clamp(Number(npc.rotationDegrees ?? npcFallback.rotationDegrees), -180, 180),
                diameterPixels: clamp(Number(npc.diameterPixels ?? npcFallback.diameterPixels), 96, 800),
                innerDiameterPercent: clamp(Number(npc.innerDiameterPercent ?? npcFallback.innerDiameterPercent), 10, 80),
                segmentGapPixels: clamp(Number(npc.segmentGapPixels ?? npcFallback.segmentGapPixels), 0, 30),
                activeColor: parseColor(npc.activeColor, npcFallback.activeColor),
                inactiveColor: parseColor(npc.inactiveColor, npcFallback.inactiveColor),
                borderColor: parseColor(npc.borderColor, npcFallback.borderColor),
                sharedSegmentImage: safeAssetPath(npc.sharedSegmentImage),
                segmentImages: Array.from({ length: 8 }, (_, index) =>
                    safeAssetPath(Array.isArray(npc.segmentImages) ? npc.segmentImages[index] : '')),
            },
        }
    } catch {
        return null
    }
}

const parseAttackWarnings = (payload: string): AttackWarning[] => {
    try {
        const value = JSON.parse(payload) as unknown
        if (!Array.isArray(value)) return []
        return value.slice(0, 32).flatMap((item) => {
            if (!item || typeof item !== 'object') return []
            const candidate = item as Partial<AttackWarning>
            const id = Math.trunc(Number(candidate.id))
            const direction = Math.trunc(Number(candidate.direction))
            if (!Number.isFinite(id) || id < 0 || direction < 0 || direction > 8) return []
            return [{
                id,
                direction,
                xPercent: clamp(Number(candidate.xPercent), 0, 100),
                yPercent: clamp(Number(candidate.yPercent), 0, 100),
                resolutionScale: clamp(Number(candidate.resolutionScale), 0.5, 4),
                distanceScale: clamp(Number(candidate.distanceScale ?? 1), 0.35, 1),
            }]
        })
    } catch {
        return []
    }
}

let settingsReceiver: ((payload: string) => void) | null = null
let directionReceiver: ((payload: string) => void) | null = null
let runtimeReceiver: ((payload: string) => void) | null = null
let attackWarningsReceiver: ((payload: string) => void) | null = null
let pendingSettings = ''
let pendingDirection = '0'
let pendingRuntime = '0|0|50|50|1|0'
let pendingAttackWarnings = '[]'

declare global {
    interface Window {
        updateHonorCombatSettings: (payload: string) => void
        updateHonorCombatDirection: (payload: string) => void
        updateHonorCombatRuntime: (payload: string) => void
        updateHonorCombatAttackWarnings: (payload: string) => void
    }
}

window.updateHonorCombatSettings = (payload: string) => {
    if (settingsReceiver) settingsReceiver(payload)
    else pendingSettings = payload
}
window.updateHonorCombatDirection = (payload: string) => {
    if (directionReceiver) directionReceiver(payload)
    else pendingDirection = payload
}
window.updateHonorCombatRuntime = (payload: string) => {
    if (runtimeReceiver) runtimeReceiver(payload)
    else pendingRuntime = payload
}
window.updateHonorCombatAttackWarnings = (payload: string) => {
    if (attackWarningsReceiver) attackWarningsReceiver(payload)
    else pendingAttackWarnings = payload
}

const colorToCss = (color: Color) => {
    const red = Math.round(clamp(color[0], 0, 1) * 255)
    const green = Math.round(clamp(color[1], 0, 1) * 255)
    const blue = Math.round(clamp(color[2], 0, 1) * 255)
    return `rgba(${red}, ${green}, ${blue}, ${clamp(color[3], 0, 1)})`
}

const supportedExtensions = ['svg', 'png', 'webp', 'jpg', 'jpeg', 'gif']

const loadFirstAsset = (candidates: string[], generation: number, currentGeneration: () => number) =>
    new Promise<string>((resolve) => {
        let index = 0
        const tryNext = () => {
            if (generation !== currentGeneration() || index >= candidates.length) {
                resolve('')
                return
            }
            const candidate = candidates[index++]
            const image = new Image()
            image.onload = () => resolve(generation === currentGeneration() ? candidate : '')
            image.onerror = tryNext
            image.src = candidate
        }
        tryNext()
    })

function App() {
    const [config, setConfig] = createSignal(defaultConfig())
    const [direction, setDirection] = createSignal(0)
    const [runtime, setRuntime] = createSignal({
        eligible: false,
        previewEligible: false,
        xPercent: 50,
        yPercent: 50,
        resolutionScale: 1,
        attached: false,
    })
    const [attackWarnings, setAttackWarnings] = createSignal<AttackWarning[]>([])
    const [segmentAssets, setSegmentAssets] = createSignal<string[]>(Array.from({ length: 8 }, () => ''))
    const [npcSegmentAssets, setNpcSegmentAssets] = createSignal<string[]>(Array.from({ length: 8 }, () => ''))
    const [centerAsset, setCenterAsset] = createSignal('')
    let assetGeneration = 0

    settingsReceiver = (payload) => {
        const parsed = parseConfig(payload)
        if (parsed) setConfig(parsed)
    }
    directionReceiver = (payload) => setDirection(clamp(parseInt(payload, 10) || 0, 0, 8))
    runtimeReceiver = (payload) => {
        const parts = payload.split('|')
        if (parts.length < 6) return
        setRuntime({
            eligible: parts[0] === '1',
            previewEligible: parts[1] === '1',
            xPercent: clamp(Number(parts[2]), 0, 100),
            yPercent: clamp(Number(parts[3]), 0, 100),
            resolutionScale: clamp(Number(parts[4]), 0.5, 4),
            attached: parts[5] === '1',
        })
    }
    attackWarningsReceiver = (payload) => setAttackWarnings(parseAttackWarnings(payload))

    if (pendingSettings) settingsReceiver(pendingSettings)
    directionReceiver(pendingDirection)
    runtimeReceiver(pendingRuntime)
    attackWarningsReceiver(pendingAttackWarnings)

    createEffect(() => {
        const current = config()
        const generation = ++assetGeneration
        const getGeneration = () => assetGeneration
        const resolveSegment = async (
            source: Pick<NpcUiConfig, 'segmentImages' | 'sharedSegmentImage'>,
            index: number, merges: boolean[], merge8With1: boolean) => {
            let principalIndex = index === 7 && merge8With1 ? 0 : index
            while (principalIndex > 0 && merges[principalIndex]) principalIndex--
            const explicit = source.segmentImages[principalIndex]
            const shared = source.sharedSegmentImage
            const candidates = explicit
                ? [explicit]
                : shared
                  ? [shared]
                  : [
                        ...supportedExtensions.map((extension) => `./Assets/HonorCombat/Default/Segment${principalIndex + 1}.${extension}`),
                        ...supportedExtensions.map((extension) => `./Assets/HonorCombat/Default/Segment.${extension}`),
                    ]
            return loadFirstAsset(candidates, generation, getGeneration)
        }
        for (let index = 0; index < 8; index++) {
            void resolveSegment(current, index, current.mergeWithPrevious, current.mergeDirection8With1).then((path) => {
                if (generation === assetGeneration) {
                    setSegmentAssets((previous) => previous.map((item, itemIndex) => itemIndex === index ? path : item))
                }
            })
            void resolveSegment(current.npc, index, current.npc.mergeWithPrevious, current.npc.mergeDirection8With1).then((path) => {
                if (generation === assetGeneration) {
                    setNpcSegmentAssets((previous) => previous.map((item, itemIndex) => itemIndex === index ? path : item))
                }
            })
        }

        const centerCandidates = current.centerImage
            ? [current.centerImage]
            : supportedExtensions.map((extension) => `./Assets/HonorCombat/Default/Center.${extension}`)
        void loadFirstAsset(centerCandidates, generation, getGeneration).then((path) => {
            if (generation === assetGeneration) setCenterAsset(path)
        })
    })

    const makeGeometry = (current: Pick<NpcUiConfig, 'diameterPixels' | 'segmentGapPixels' | 'innerDiameterPercent'>) => {
        const pixelToViewBox = 1000 / current.diameterPixels
        const gap = current.segmentGapPixels * pixelToViewBox
        const outer = Math.max(220, 470 - gap * 0.5)
        const inner = Math.min(outer - 24, 500 * current.innerDiameterPercent / 100 + gap * 0.5)
        const halfAngle = Math.max(4, 22.5 - (gap / Math.max(outer, 1)) * 90 / Math.PI)
        const radians = (angle: number) => angle * Math.PI / 180
        const point = (radius: number, angle: number) => ({
            x: 500 + radius * Math.cos(radians(angle)),
            y: 500 + radius * Math.sin(radians(angle)),
        })
        const outerLeft = point(outer, -90 - halfAngle)
        const outerRight = point(outer, -90 + halfAngle)
        const innerRight = point(inner, -90 + halfAngle)
        const innerLeft = point(inner, -90 - halfAngle)
        const path = [
            `M ${outerLeft.x} ${outerLeft.y}`,
            `A ${outer} ${outer} 0 0 1 ${outerRight.x} ${outerRight.y}`,
            `L ${innerRight.x} ${innerRight.y}`,
            `A ${inner} ${inner} 0 0 0 ${innerLeft.x} ${innerLeft.y}`,
            'Z',
        ].join(' ')
        const imageX = outerLeft.x
        const imageY = 500 - outer
        const imageWidth = outerRight.x - outerLeft.x
        const imageHeight = innerLeft.y - imageY
        return { path, inner, imageX, imageY, imageWidth, imageHeight }
    }
    const geometry = createMemo(() => makeGeometry(config()))
    const npcGeometry = createMemo(() => makeGeometry(config().npc))

    const selectedDirection = createMemo(() => direction())
    const centerRadius = createMemo(() =>
        Math.max(1, (geometry().inner - 5) * config().centerPieceSizePercent / 100))
    const principalDirection = (index: number) => {
        if (index === 7 && config().mergeDirection8With1) return 1
        const merges = config().mergeWithPrevious
        while (index > 0 && merges[index]) index--
        return index + 1
    }
    const npcPrincipalDirection = (index: number) => {
        if (index === 7 && config().npc.mergeDirection8With1) return 1
        const merges = config().npc.mergeWithPrevious
        while (index > 0 && merges[index]) index--
        return index + 1
    }
    const centerOffset = createMemo(() => {
        if (!config().moveCenterWithDirection || selectedDirection() === 0) return { x: 0, y: 0 }
        const angle = (selectedDirection() - 1) * Math.PI / 4 - Math.PI / 2
        const distance = config().centerMovementPixels
        return {
            x: Math.cos(angle) * distance,
            y: Math.sin(angle) * distance,
        }
    })
    const horizontalPosition = createMemo(() => runtime().attached ? runtime().xPercent : config().positionXPercent)
    const verticalPosition = createMemo(() => runtime().attached ? runtime().yPercent : config().positionYPercent)
    const effectiveScale = createMemo(() => config().scalePercent / 100 * runtime().resolutionScale)

    return (
        <>
            <Show when={config().enabled && (runtime().eligible || (config().editMode && runtime().previewEligible))}>
                <div
                    class="honor-hud"
                    style={{
                        left: `${horizontalPosition()}%`,
                        top: `${verticalPosition()}%`,
                        width: `${config().diameterPixels}px`,
                        height: `${config().diameterPixels}px`,
                        opacity: `${config().opacityPercent / 100}`,
                        transform: `translate(-50%, -50%) scale(${effectiveScale()})`,
                    }}
                >
                    <svg viewBox="0 0 1000 1000" role="presentation" aria-hidden="true">
                <defs>
                    <clipPath id="honor-segment-clip">
                        <path d={geometry().path} />
                    </clipPath>
                    <clipPath id="honor-center-clip">
                        <circle cx="500" cy="500" r={centerRadius()} />
                    </clipPath>
                </defs>
                <For each={Array.from({ length: 8 }, (_, index) => index)}>
                    {(index) => {
                        const active = () => selectedDirection() === principalDirection(index)
                        return (
                            <g transform={`rotate(${index * 45 + config().rotationDegrees} 500 500)`}>
                                <path
                                    class="direction-segment"
                                    d={geometry().path}
                                    fill={colorToCss(active() ? config().activeColor : config().inactiveColor)}
                                    stroke={colorToCss(config().borderColor)}
                                    stroke-width="5"
                                    vector-effect="non-scaling-stroke"
                                />
                                <Show when={segmentAssets()[index]}>
                                    <image
                                        class="direction-image"
                                        classList={{ active: active() }}
                                        href={segmentAssets()[index]}
                                        x={geometry().imageX}
                                        y={geometry().imageY}
                                        width={geometry().imageWidth}
                                        height={geometry().imageHeight}
                                        preserveAspectRatio="xMidYMid slice"
                                        clip-path="url(#honor-segment-clip)"
                                    />
                                </Show>
                            </g>
                        )
                    }}
                </For>
                <Show when={config().showCenter}>
                    <g
                        class="center-piece-group"
                        style={{
                            transform: `translate(${centerOffset().x}px, ${centerOffset().y}px)`,
                            'transition-duration': `${config().centerMovementDurationMs}ms`,
                        }}
                    >
                        <circle
                            class="center-piece"
                            cx="500"
                            cy="500"
                            r={centerRadius()}
                            fill={colorToCss(config().centerColor)}
                            stroke={colorToCss(config().borderColor)}
                            stroke-width="5"
                        />
                        <Show when={centerAsset()}>
                            <image
                                class="center-image"
                                href={centerAsset()}
                                x={500 - centerRadius()}
                                y={500 - centerRadius()}
                                width={centerRadius() * 2}
                                height={centerRadius() * 2}
                                preserveAspectRatio="xMidYMid slice"
                                clip-path="url(#honor-center-clip)"
                            />
                        </Show>
                    </g>
                </Show>
                    </svg>
                </div>
            </Show>

            <Show when={config().npc.enabled}>
            <Index each={attackWarnings()}>
                {(warning) => {
                    const clipId = () => `honor-attack-warning-${warning().id}`
                    const warningScale = () =>
                        config().npc.scalePercent / 100 * warning().resolutionScale * warning().distanceScale
                    return (
                        <div
                            class="honor-hud attack-warning"
                            data-threat-id={warning().id}
                            style={{
                                left: `calc(${warning().xPercent}% + ${config().npc.offsetXPixels * warning().resolutionScale}px)`,
                                top: `calc(${warning().yPercent}% - ${config().npc.offsetYPixels * warning().resolutionScale}px)`,
                                width: `${config().npc.diameterPixels}px`,
                                height: `${config().npc.diameterPixels}px`,
                                opacity: `${config().npc.opacityPercent / 100}`,
                                transform: `translate(-50%, -50%) scale(${warningScale()})`,
                            }}
                        >
                            <svg viewBox="0 0 1000 1000" role="presentation" aria-hidden="true">
                                <defs>
                                    <clipPath id={clipId()}>
                                        <path d={npcGeometry().path} />
                                    </clipPath>
                                </defs>
                                <For each={Array.from({ length: 8 }, (_, index) => index)}>
                                    {(index) => {
                                        const active = () => warning().direction === npcPrincipalDirection(index)
                                        return (
                                            <g transform={`rotate(${index * 45 + config().npc.rotationDegrees} 500 500)`}>
                                                <path
                                                    class="direction-segment attack-warning-segment"
                                                    classList={{ active: active() }}
                                                    d={npcGeometry().path}
                                                    fill={colorToCss(active() ? config().npc.activeColor : config().npc.inactiveColor)}
                                                    stroke={colorToCss(config().npc.borderColor)}
                                                    stroke-width="5"
                                                    vector-effect="non-scaling-stroke"
                                                />
                                                <Show when={npcSegmentAssets()[index]}>
                                                    <image
                                                        class="direction-image attack-warning-image"
                                                        classList={{ active: active() }}
                                                        href={npcSegmentAssets()[index]}
                                                        x={npcGeometry().imageX}
                                                        y={npcGeometry().imageY}
                                                        width={npcGeometry().imageWidth}
                                                        height={npcGeometry().imageHeight}
                                                        preserveAspectRatio="xMidYMid slice"
                                                        clip-path={`url(#${clipId()})`}
                                                    />
                                                </Show>
                                            </g>
                                        )
                                    }}
                                </For>
                            </svg>
                        </div>
                    )
                }}
            </Index>
            </Show>
        </>
    )
}

export default App
