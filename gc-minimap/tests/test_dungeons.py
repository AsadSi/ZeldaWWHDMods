"""Authored room resources only: no game data or screenshots in fixtures."""
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import dungeons
from test_formats import bti, dzr


def room_dzr(bounds=(-10., -20., 30., 40.), floor=128):
    data = bytearray(dzr(bounds, floor))
    struct.pack_into('>4f', data, 16 + 0x18, *bounds)
    return bytes(data)


class DungeonTests(unittest.TestCase):
    def room(self, floor=128):
        return {'dat/room.dzr': room_dzr(floor=floor), f'dat/s{floor}.bti': bti(),
                f'dat/m{floor}.bti': bti(), f'dat/m{floor}.amp': bytes(60)}

    def test_current_room_requires_all_resources(self):
        room = self.room()
        self.assertEqual(dungeons.room_records(room)[0][:2], (128, (-10., -20., 30., 40.)))
        for name in room:
            bad = dict(room); del bad[name]
            self.assertEqual(dungeons.room_records(bad), [])

    def test_bounds_inherit_downward_and_floors_stay_sorted(self):
        room = self.room(127)
        room.update({f'dat/{p}129.{ext}': bti() if ext == 'bti' else bytes(60)
                     for p, ext in (('s', 'bti'), ('m', 'bti'), ('m', 'amp'))})
        self.assertEqual([r[0] for r in dungeons.room_records(room)], [127, 129])
        room['dat/room.dzr'] = room_dzr(floor=130)
        self.assertEqual(dungeons.room_records(room), [])

    def test_invalid_detailed_map_hides_corner_map(self):
        room = self.room()
        room['dat/room.dzr'] = dzr()  # no detailed bounds
        self.assertEqual(dungeons.room_records(room), [])
        room = self.room()
        room['dat/m128.bti'] = bti(kind=9)  # 8 x 4: not a GC detailed map shape
        self.assertEqual(dungeons.room_records(room), [])
        room = self.room(); room['dat/m128.amp'] = b''
        with self.assertRaises(ValueError): dungeons.room_records(room)

    def test_degenerate_bounds_hide_room(self):
        room = self.room(); room['dat/room.dzr'] = dzr((0, 0, 0, 0))
        self.assertEqual(dungeons.room_records(room), [])

    def test_failure_keeps_global_readiness_disabled(self):
        class Context:
            def __init__(self): self.writes = []
            def write_data(self, name, data): self.writes.append((name, data))
            def read_game(self, offset, size): return b'bad'
        context = Context()
        with self.assertRaises(ValueError):
            dungeons.prepare(context, {'res/Stage/Test/Room0.arc': (0, 3)})
        self.assertEqual(context.writes, [('dungeons-ready.bin', b'')])

    def test_disc_adapter_and_binary_receipt(self):
        class Context:
            def __init__(self): self.writes = {}
            def write_data(self, name, data): self.writes[name] = data
        class Disc:
            def read_game(self, offset, size): return b'archive'
        context = Context()
        with patch.object(dungeons.formats, 'rarc_files', return_value=self.room()), \
             patch.object(dungeons.vector, 'trace', return_value=(bytes(4), b'', {})), \
             patch.object(dungeons.formats, 'png_rgba', return_value=b'png'):
            rooms = dungeons.prepare(context, {'files/res/Stage/Test/Room0.arc': (20, 7)}, Disc())
        self.assertEqual(rooms, [{'stage': 'Test', 'room': 0, 'floors': [128]}])
        self.assertEqual(context.writes['d-Test-00.bin'], b'GCR1' + struct.pack('>II4f', 1, 128, -10, -20, 30, 40))
        self.assertEqual(context.writes['dungeons-ready.bin'], b'GCR1' + struct.pack('>I8sII', 1, b'Test', 1, 0))

    def test_floor_membership_and_room_override(self):
        def flor(entries):
            body = b''
            for floor, rooms in entries:
                body += struct.pack('>fB14bB', 0, floor, *(rooms + [-1] * (14 - len(rooms))), 0)
            return struct.pack('>I4sII', 1, b'FLOR', len(entries), 16) + body
        stage = dungeons.floor_rooms(flor([(128, [0]), (129, [1])]))
        self.assertEqual(dungeons.room_membership(self.room(), stage, 0), [128])
        self.assertEqual(dungeons.room_membership(self.room(), stage, 1), [129])
        self.assertEqual(dungeons.room_membership(self.room(), stage, 2), [])
        local = {'dat/room.dzr': flor([(127, [2])])}
        self.assertEqual(dungeons.room_membership(local, stage, 2), [127])
        self.assertEqual(dungeons.room_membership(self.room(), None, 0), [128])
        with self.assertRaises(ValueError): dungeons.floor_rooms(flor([(128, [0]), (128, [1])]))
        with self.assertRaises(ValueError): dungeons.floor_rooms(flor([(128, [0])])[:-1])

    def test_registered_floor_can_reuse_lower_resource(self):
        records = dungeons.room_records(self.room())
        displayed = dungeons.display_records(records, [127, 128, 129])
        self.assertEqual([floor for floor, _ in displayed], [128, 129])
        self.assertEqual([selected[0] for _, selected in displayed], [128, 128])

    def test_new_dump_excludes_stale_room_files(self):
        class Context:
            def __init__(self): self.writes = {'d-Old-00.bin': b'old-map'}
            def write_data(self, name, data): self.writes[name] = data
        context = Context()
        self.assertEqual(dungeons.prepare(context, {}), [])
        self.assertEqual(context.writes['d-Old-00.bin'], b'old-map')
        self.assertEqual(context.writes['dungeons-ready.bin'], b'GCR1' + bytes(4))

    def test_extracted_folder_accepts_files_prefix(self):
        class Context:
            def read_input(self, name):
                if not name.startswith('files/'): raise FileNotFoundError(name)
                return b'archive'
        self.assertEqual(dungeons.read_archive(Context(), None, 'res/Stage/Test/Room0.arc'), b'archive')
