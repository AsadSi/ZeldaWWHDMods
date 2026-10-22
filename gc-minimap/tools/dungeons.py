"""Local GameCube current-room maps. No game-derived bytes belong in packages."""
import re
import struct
import formats
import vector

# Retail stages with corner-map resources, plus tower stages checked explicitly.
STAGES = ('A_nami Cave08 DmSpot0 E3ROOP KATA_RM M_Dai M_DaiB M_DaiMB '
          'M_Dra09 M_DragB M_NewD2 MajyuE Ocean Siren SirenB SirenMB VrTest '
          'kaze kazeB kazeMB kinBOSS kinMB kindan ma2room ma3room majroom '
          'GanonA GanonB GanonC GanonD GanonE GanonJ GanonK GanonL GanonM GanonN GTower').split()


def read_archive(context, files, relative):
    if files is None:
        try:
            return context.read_input(relative)
        except FileNotFoundError:
            return context.read_input('files/' + relative)
    name = relative if relative in files else 'files/' + relative
    if name not in files:
        raise FileNotFoundError(relative)
    return context.read_game(*files[name])


def floor_rooms(data):
    """FLOR membership controls which images GC registers for each room."""
    count = formats.u32(data, 0)
    if count > 1024:
        raise ValueError('Invalid floor chunk count')
    formats.span(data, 4, count * 12)
    found = None
    for i in range(count):
        tag, number, offset = struct.unpack('>4sII', formats.span(data, 4 + i * 12, 12))
        if tag != b'FLOR':
            continue
        if found is not None or number > 10 or offset < 4 + count * 12:
            raise ValueError('Invalid floor table')
        entries = formats.span(data, offset, number * 20)
        found = {}
        for n in range(number):
            record = entries[n * 20:(n + 1) * 20]
            floor = record[4]
            if floor < 123 or floor > 132 or floor in found:
                raise ValueError('Invalid or duplicate dungeon floor')
            found[floor] = set(struct.unpack('>14b', record[5:19]))
    return found


def room_membership(archive, stage_floors, room):
    dzrs = [v for k, v in archive.items() if k in ('dat/room.dzr', 'dzr/room.dzr', 'room.dzr')]
    if len(dzrs) != 1:
        return []
    local = floor_rooms(dzrs[0])
    floors = stage_floors if local is None else local
    if floors is None:
        return [128]  # GC registers only 1F when neither stage nor room has FLOR.
    return sorted(floor for floor, rooms in floors.items() if room in rooms)


def display_records(records, floors):
    # getRoomImage searches downward. The current room's getRoomDspFloorNo
    # searches registered entries downward, then upward (the guest decoder).
    result = []
    for floor in floors:
        choices = [record for record in records if record[0] <= floor]
        if choices:
            selected = choices[-1]
            result.append((floor, selected))
    return result


def room_records(archive):
    """Require the same three resources as GC getRoomImage; select verified 2DMA."""
    dzrs = [v for k, v in archive.items() if k in ('dat/room.dzr', 'dzr/room.dzr', 'room.dzr')]
    if len(dzrs) != 1:
        return []
    records = []
    for floor in range(123, 133):
        small, detail, amp = (f'dat/s{floor}.bti', f'dat/m{floor}.bti', f'dat/m{floor}.amp')
        if not all(k in archive for k in (small, detail, amp)):
            continue
        # Bounds may be inherited from the next lower 2DMA floor, as in GC.
        bounds = None
        for probe in range(floor, 122, -1):
            try:
                bounds = formats.bounds_from_dzr(dzrs[0], probe)
                break
            except formats.MissingBounds:
                pass
            except ValueError:
                return []  # Invalid existing 2DMA must not inherit another floor's bounds.
        if bounds is None:
            continue
        try:
            formats.bounds_from_dzr(dzrs[0], probe, detail=True)
        except ValueError:
            return []
        formats.span(archive[amp], 0, 60)
        dw, dh = struct.unpack('>HH', formats.span(archive[detail], 2, 4))
        if not ((dw == 128 and 0 < dh <= 480) or (dw > 0 and dw % 8 == 0 and dh == 8)):
            return []
        width, height, rgba = formats.decode_bti(archive[small])
        records.append((floor, bounds, width, height, rgba))
    return records


def prepare(context, files, disc=None):
    source = disc if disc is not None else context
    context.write_data('dungeons-ready.bin', b'')
    if files is None:
        candidates = [(stage, room, f'res/Stage/{stage}/Room{room}.arc')
                      for stage in STAGES for room in range(64)]
    else:
        candidates = []
        for name in sorted(files):
            match = re.fullmatch(r'(?:files/)?res/Stage/([A-Za-z0-9_]{1,8})/Room([0-9]{1,2})\.arc', name)
            if match and match[1] not in ('sea', 'sea_T', 'sea_E') and int(match[2]) < 64:
                candidates.append((match[1], int(match[2]), name.removeprefix('files/')))
    rooms = []
    stage_floors = {}
    for stage, room, relative in candidates:
        try:
            archive = formats.rarc_files(read_archive(source, files, relative))
        except FileNotFoundError:
            continue
        records = room_records(archive)
        if not records:
            continue
        if stage not in stage_floors:
            try:
                stage_archive = formats.rarc_files(read_archive(source, files, f'res/Stage/{stage}/Stage.arc'))
            except FileNotFoundError:
                stage_archive = {}
            dzs = [v for k, v in stage_archive.items() if k in ('dat/stage.dzs', 'dzs/stage.dzs', 'stage.dzs')]
            stage_floors[stage] = floor_rooms(dzs[0]) if len(dzs) == 1 else None
        displayed = display_records(records, room_membership(archive, stage_floors[stage], room))
        if not displayed:
            continue
        prefix = f'd-{stage}-{room:02}'
        metadata = bytearray(b'GCR1' + struct.pack('>I', len(displayed)))
        textures = {}
        for floor, selected in displayed:
            resource, bounds, width, height, rgba = selected
            metadata.extend(struct.pack('>I4f', floor, *bounds))
            if resource not in textures:
                raster, _, _ = vector.trace(width, height, rgba, size=512)
                textures[resource] = (formats.png_rgba(width, height, rgba), formats.png_rgba(512, 512, raster))
            original, traced = textures[resource]
            context.write_data(f'{prefix}-{floor}-original.png', original)
            context.write_data(f'{prefix}-{floor}-vector.png', traced)
        context.write_data(prefix + '.bin', bytes(metadata))
        rooms.append({'stage': stage, 'room': room, 'floors': [r[0] for r in displayed]})
    # Committed last: a failed preparation cannot revive stale maps from another dump.
    stages = {}
    for item in rooms:
        masks = stages.setdefault(item['stage'], [0, 0])
        masks[item['room'] // 32] |= 1 << (item['room'] % 32)
    if len(stages) > 64:
        raise ValueError('Too many dungeon stages')
    ready = b'GCR1' + struct.pack('>I', len(stages))
    for stage, masks in sorted(stages.items()):
        ready += struct.pack('>8sII', stage.encode('ascii'), *masks)
    context.write_data('dungeons-ready.bin', ready)
    return rooms
