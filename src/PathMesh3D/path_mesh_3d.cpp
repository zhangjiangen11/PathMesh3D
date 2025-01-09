#include "path_mesh_3d.hpp"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/classes/material.hpp>

using namespace godot;

#define CHECK_SURFACE_IDX(m_idx) ERR_FAIL_COND(m_idx < 0 || m_idx >= surfaces.size())
#define CHECK_SURFACE_IDX_V(m_idx, m_ret) ERR_FAIL_COND_V(m_idx < 0 || m_idx >= surfaces.size(), m_ret)

void PathMesh3D::set_distribution(uint64_t p_surface_idx, Distribution p_distribution) {
    CHECK_SURFACE_IDX(p_surface_idx);

    if (surfaces[p_surface_idx].distribution != p_distribution) {
        surfaces.write[p_surface_idx].distribution = p_distribution;
        _queue_surface(p_surface_idx);
        notify_property_list_changed();
    }
}

PathMesh3D::Distribution PathMesh3D::get_distribution(uint64_t p_surface_idx) const {
    CHECK_SURFACE_IDX_V(p_surface_idx, Distribution::DISTRIBUTE_MAX);

    return surfaces[p_surface_idx].distribution;
}

void PathMesh3D::set_alignment(uint64_t p_surface_idx, Alignment p_alignment) {
    CHECK_SURFACE_IDX(p_surface_idx);
    
    if (surfaces[p_surface_idx].alignment != p_alignment) {
        surfaces.write[p_surface_idx].alignment = p_alignment;
        _queue_surface(p_surface_idx);
    }
}

PathMesh3D::Alignment PathMesh3D::get_alignment(uint64_t p_surface_idx) const {
    CHECK_SURFACE_IDX_V(p_surface_idx, Alignment::ALIGN_MAX);

    return surfaces[p_surface_idx].alignment;
}

void PathMesh3D::set_count(uint64_t p_surface_idx, uint64_t p_count) {
    CHECK_SURFACE_IDX(p_surface_idx);
    p_count = Math::clamp(p_count, 2ULL, _get_max_count());

    if (surfaces[p_surface_idx].count != p_count) {
        surfaces.write[p_surface_idx].count = p_count;
        if (surfaces[p_surface_idx].distribution == DISTRIBUTE_BY_COUNT) {
            _queue_surface(p_surface_idx);
        }
    }
}

uint64_t PathMesh3D::get_count(uint64_t p_surface_idx) const {
    CHECK_SURFACE_IDX_V(p_surface_idx, 0);

    return surfaces[p_surface_idx].distribution == Distribution::DISTRIBUTE_BY_COUNT ?
        surfaces[p_surface_idx].count : 0;
}

void PathMesh3D::set_warp_along_curve(uint64_t p_surface_idx, bool p_warp) {
    CHECK_SURFACE_IDX(p_surface_idx);

    if (surfaces[p_surface_idx].warp_along_curve != p_warp) {
        surfaces.write[p_surface_idx].warp_along_curve = p_warp;
        _queue_surface(p_surface_idx);
    }
}

bool PathMesh3D::get_warp_along_curve(uint64_t p_surface_idx) const {
    CHECK_SURFACE_IDX_V(p_surface_idx, false);
    
    return surfaces[p_surface_idx].warp_along_curve;
}

void PathMesh3D::set_sample_cubic(uint64_t p_surface_idx, bool p_cubic) {
    CHECK_SURFACE_IDX(p_surface_idx);

    if (surfaces[p_surface_idx].cubic != p_cubic) {
        surfaces.write[p_surface_idx].cubic = p_cubic;
        _queue_surface(p_surface_idx);
    }
}

bool PathMesh3D::get_sample_cubic(uint64_t p_surface_idx) const {
    CHECK_SURFACE_IDX_V(p_surface_idx, false);

    return surfaces[p_surface_idx].cubic;
}

void PathMesh3D::set_tilt(uint64_t p_surface_idx, bool p_tilt) {
    CHECK_SURFACE_IDX(p_surface_idx);

    if (surfaces[p_surface_idx].tilt != p_tilt) {
        surfaces.write[p_surface_idx].tilt = p_tilt;
        _queue_surface(p_surface_idx);
    }
}

bool PathMesh3D::get_tilt(uint64_t p_surface_idx) const {
    CHECK_SURFACE_IDX_V(p_surface_idx, false);

    return surfaces[p_surface_idx].tilt;
}

void PathMesh3D::set_offset(uint64_t p_surface_idx, Vector2 p_offset) {
    CHECK_SURFACE_IDX(p_surface_idx);

    if (surfaces[p_surface_idx].offset != p_offset) {
        surfaces.write[p_surface_idx].offset = p_offset;
        _queue_surface(p_surface_idx);
    }
}

Vector2 PathMesh3D::get_offset(uint64_t p_surface_idx) const {
    CHECK_SURFACE_IDX_V(p_surface_idx, Vector2());

    return surfaces[p_surface_idx].offset;
}

void PathMesh3D::set_mesh(const Ref<Mesh> &p_mesh) {
    if (p_mesh != source_mesh) {
        if (source_mesh.is_valid() && source_mesh->is_connected("changed", callable_mp(this, &PathMesh3D::_on_mesh_changed))) {
            source_mesh->disconnect("changed", callable_mp(this, &PathMesh3D::_on_mesh_changed));
        }

        source_mesh = p_mesh;

        if (source_mesh.is_valid()) {
            source_mesh->connect("changed", callable_mp(this, &PathMesh3D::_on_mesh_changed));
            _on_mesh_changed();
        } else {
            surfaces.clear();
            set_base(RID());
        }

        notify_property_list_changed();
    }
}

Ref<Mesh> PathMesh3D::get_mesh() const {
    return source_mesh;
}

void PathMesh3D::set_path_3d(Path3D *p_path) {
    if (p_path != path3d) {
        if (path3d != nullptr && path3d->is_connected("curve_changed", callable_mp(this, &PathMesh3D::_on_curve_changed))) {
            path3d->disconnect("curve_changed", callable_mp(this, &PathMesh3D::_on_curve_changed));
        }

        path3d = p_path;

        if (path3d != nullptr) {
            path3d->connect("curve_changed", callable_mp(this, &PathMesh3D::_on_curve_changed));
            _on_curve_changed();
        }
    }
}

Path3D *PathMesh3D::get_path_3d() const {
    return path3d;
}

void PathMesh3D::queue_rebuild() {
    for (SurfaceData &surf : surfaces) {
        surf.dirty = true;
    }
    callable_mp(this, &PathMesh3D::_rebuild_mesh).call_deferred();
}

Ref<ArrayMesh> PathMesh3D::get_baked_mesh() const {
    return generated_mesh->duplicate();
}

void PathMesh3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("queue_rebuild"), &PathMesh3D::queue_rebuild);
    ClassDB::bind_method(D_METHOD("get_baked_mesh"), &PathMesh3D::get_baked_mesh);

    ClassDB::bind_method(D_METHOD("set_distribution", "surface_index", "distribution"), &PathMesh3D::set_distribution);
    ClassDB::bind_method(D_METHOD("get_distribution", "surface_index"), &PathMesh3D::get_distribution);
    ClassDB::bind_method(D_METHOD("set_alignment", "surface_index", "alignment"), &PathMesh3D::set_alignment);
    ClassDB::bind_method(D_METHOD("get_alignment", "surface_index"), &PathMesh3D::get_alignment);
    ClassDB::bind_method(D_METHOD("set_count", "surface_index", "count"), &PathMesh3D::set_count);
    ClassDB::bind_method(D_METHOD("get_count", "surface_index"), &PathMesh3D::get_count);
    ClassDB::bind_method(D_METHOD("set_warp_along_curve", "surface_index", "warp"), &PathMesh3D::set_warp_along_curve);
    ClassDB::bind_method(D_METHOD("get_warp_along_curve", "surface_index"), &PathMesh3D::get_warp_along_curve);
    ClassDB::bind_method(D_METHOD("set_sample_cubic", "surface_index", "cubic"), &PathMesh3D::set_sample_cubic);
    ClassDB::bind_method(D_METHOD("get_sample_cubic", "surface_index"), &PathMesh3D::get_sample_cubic);
    ClassDB::bind_method(D_METHOD("set_tilt", "surface_index", "tilt"), &PathMesh3D::set_tilt);
    ClassDB::bind_method(D_METHOD("get_tilt", "surface_index"), &PathMesh3D::get_tilt);
    ClassDB::bind_method(D_METHOD("set_offset", "surface_index", "offset"), &PathMesh3D::set_offset);
    ClassDB::bind_method(D_METHOD("get_offset", "surface_index"), &PathMesh3D::get_offset);

    ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &PathMesh3D::set_mesh);
    ClassDB::bind_method(D_METHOD("get_mesh"), &PathMesh3D::get_mesh);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_mesh", "get_mesh");

    ClassDB::bind_method(D_METHOD("set_path_3d", "path"), &PathMesh3D::set_path_3d);
    ClassDB::bind_method(D_METHOD("get_path_3d"), &PathMesh3D::get_path_3d);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "path_3d", PROPERTY_HINT_NODE_TYPE, "Path3D"), "set_path_3d", "get_path_3d");
    
    ADD_SIGNAL(MethodInfo("mesh_changed"));
    ADD_SIGNAL(MethodInfo("curve_changed"));

    BIND_ENUM_CONSTANT(DISTRIBUTE_BY_COUNT);
    BIND_ENUM_CONSTANT(DISTRIBUTE_BY_MODEL_LENGTH);
    BIND_ENUM_CONSTANT(ALIGN_STRETCH);
    BIND_ENUM_CONSTANT(ALIGN_FROM_START);
    BIND_ENUM_CONSTANT(ALIGN_CENTERED);
    BIND_ENUM_CONSTANT(ALIGN_FROM_END);
}

void PathMesh3D::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_POST_ENTER_TREE: {
            generated_mesh.instantiate();
            set_base(generated_mesh->get_rid());
            queue_rebuild();
        } break;
    }
}

void PathMesh3D::_get_property_list(List<PropertyInfo> *p_list) const {
    for (uint64_t idx = 0; idx < surfaces.size(); ++idx) {
        const SurfaceData &surf = surfaces[idx];

        String surf_name = "surface_" + itos(idx);
        uint32_t usage = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_INTERNAL;
            
        p_list->push_back(PropertyInfo(
            Variant::NIL, surf_name.capitalize(), PROPERTY_HINT_NONE, surf_name + "/", PROPERTY_USAGE_GROUP));
        p_list->push_back(PropertyInfo(
                Variant::INT, surf_name + "/distribution", PROPERTY_HINT_ENUM, "By Model Length,By Count", usage));
        p_list->push_back(PropertyInfo(
                Variant::INT, surf_name + "/alignment", PROPERTY_HINT_ENUM, "Stretch,From Start,Center,From End", usage));
        if (surf.distribution == Distribution::DISTRIBUTE_BY_COUNT) {
            String hint_str = "2," + itos(_get_max_count()) + ",1,or_greater";
            p_list->push_back(PropertyInfo(
                    Variant::INT, surf_name + "/count", PROPERTY_HINT_RANGE, hint_str, usage));
        }
        p_list->push_back(PropertyInfo(
                Variant::BOOL, surf_name + "/warp_along_curve", PROPERTY_HINT_NONE, "", usage));
        p_list->push_back(PropertyInfo(
                Variant::BOOL, surf_name + "/sample_cubic", PROPERTY_HINT_NONE, "", usage));
        p_list->push_back(PropertyInfo(
                Variant::BOOL, surf_name + "/tilt", PROPERTY_HINT_NONE, "", usage));
        p_list->push_back(PropertyInfo(
                Variant::VECTOR2, surf_name + "/offset", PROPERTY_HINT_NONE, "", usage));
    }
}

bool PathMesh3D::_property_can_revert(const StringName &p_name) const {
    if (p_name.begins_with("surface_")) {
        return true;
    }

    return false;
}

bool PathMesh3D::_property_get_revert(const StringName &p_name, Variant &r_property) const {
    if (p_name.begins_with("surface_")) {
        Pair<uint64_t, String> subprop = _decode_dynamic_propname(p_name);
        uint64_t surf_idx = subprop.first;
        String sub_name = subprop.second;
        if (sub_name == "distribution") {
            r_property = Distribution::DISTRIBUTE_BY_MODEL_LENGTH;
        } else if (sub_name == "alignment") {
            r_property = Alignment::ALIGN_STRETCH;
        } else if (sub_name == "count") {
            r_property = 2;
        } else if (sub_name == "warp_along_curve") {
            r_property = true;
        } else if (sub_name == "sample_cubic") {
            r_property = false;
        } else if (sub_name == "tilt") {
            r_property = true;
        } else if (sub_name == "offset") {
            r_property = Vector2();
        } else {
            return false;
        }
        return true;
    }
    return false;
}

bool PathMesh3D::_set(const StringName &p_name, const Variant &p_property) {
    if (p_name.begins_with("surface_")) {
        Pair<uint64_t, String> subprop = _decode_dynamic_propname(p_name);
        uint64_t surf_idx = subprop.first;
        String sub_name = subprop.second;
        if (sub_name == "distribution") {
            set_distribution(surf_idx, Distribution(int(p_property)));
        } else if (sub_name == "alignment") {
            set_alignment(surf_idx, Alignment(int(p_property)));
        } else if (sub_name == "count") {
            set_count(surf_idx, p_property);
        } else if (sub_name == "warp_along_curve") {
            set_warp_along_curve(surf_idx, p_property);
        } else if (sub_name == "sample_cubic") {
            set_sample_cubic(surf_idx, p_property);
        } else if (sub_name == "tilt") {
            set_tilt(surf_idx, p_property);
        } else if (sub_name == "offset") {
            set_offset(surf_idx, p_property);
        } else {
            return false;
        }
        return true;
    }
    return false;
}

bool PathMesh3D::_get(const StringName &p_name, Variant &r_property) const {
    if (p_name.begins_with("surface_")) {
        Pair<uint64_t, String> subprop = _decode_dynamic_propname(p_name);
        uint64_t surf_idx = subprop.first;
        String sub_name = subprop.second;
        if (sub_name == "distribution") {
            r_property = get_distribution(surf_idx);
        } else if (sub_name == "alignment") {
            r_property = get_alignment(surf_idx);
        } else if (sub_name == "count") {
            r_property = get_count(surf_idx);
        } else if (sub_name == "warp_along_curve") {
            r_property = get_warp_along_curve(surf_idx);
        } else if (sub_name == "sample_cubic") {
            r_property = get_sample_cubic(surf_idx);
        } else if (sub_name == "tilt") {
            r_property = get_tilt(surf_idx);
        } else if (sub_name == "offset") {
            r_property = get_offset(surf_idx);
        } else {
            return false;
        }
        return true;
    }
    return false;
}

void PathMesh3D::_queue_surface(uint64_t p_surface_idx) {
    ERR_FAIL_COND(p_surface_idx < 0 || p_surface_idx >= surfaces.size());

    surfaces.write[p_surface_idx].dirty = true;
    callable_mp(this, &PathMesh3D::_rebuild_mesh).call_deferred();
}

void PathMesh3D::_rebuild_mesh() {
    if (path3d == nullptr || path3d->get_curve().is_null() || source_mesh.is_null() || !_are_any_dirty()) {
        return;
    }

    generated_mesh->clear_surfaces();
    Ref<Curve3D> curve = path3d->get_curve();

    if (curve->get_point_count() < 2) {
        return;
    }

    double baked_l = curve->get_baked_length();
    double mesh_l = source_mesh->get_aabb().size.z;
    if (baked_l < mesh_l) {
        return;
    }
    for (uint64_t idx_surf = 0; idx_surf < source_mesh->get_surface_count(); ++idx_surf) {
        SurfaceData &surf = surfaces.write[idx_surf];
        if (!surf.dirty) {
            continue;
        }
        surf.dirty = false;

        Array arrays = source_mesh->surface_get_arrays(idx_surf);

        PackedVector3Array old_verts = arrays[Mesh::ARRAY_VERTEX];

        Vector<bool> has_column;
        has_column.resize(Mesh::ARRAY_MAX);
        for (int idx_type = 0; idx_type < Mesh::ARRAY_MAX; ++idx_type) {
            has_column.write[idx_type] = arrays[idx_type].get_type() != Variant::NIL;
        }

        #define MAKE_OLD_ARRAY(m_type, m_name, m_index) \
            m_type m_name = has_column[m_index] ? arrays[m_index] : m_type()
        MAKE_OLD_ARRAY(PackedVector3Array, old_norms, Mesh::ARRAY_NORMAL);
        MAKE_OLD_ARRAY(PackedFloat32Array, old_tang, Mesh::ARRAY_TANGENT);
        MAKE_OLD_ARRAY(PackedVector2Array, old_uv1, Mesh::ARRAY_TEX_UV);
        MAKE_OLD_ARRAY(PackedVector2Array, old_uv2, Mesh::ARRAY_TEX_UV2);
        MAKE_OLD_ARRAY(PackedColorArray, old_colors, Mesh::ARRAY_COLOR);
        MAKE_OLD_ARRAY(PackedByteArray, old_custom0, Mesh::ARRAY_CUSTOM0);
        MAKE_OLD_ARRAY(PackedByteArray, old_custom1, Mesh::ARRAY_CUSTOM1);
        MAKE_OLD_ARRAY(PackedByteArray, old_custom2, Mesh::ARRAY_CUSTOM2);
        MAKE_OLD_ARRAY(PackedByteArray, old_custom3, Mesh::ARRAY_CUSTOM3);
        MAKE_OLD_ARRAY(PackedInt32Array, old_bones /*my nickname in highschool*/, Mesh::ARRAY_BONES);
        MAKE_OLD_ARRAY(PackedFloat32Array, old_weights, Mesh::ARRAY_WEIGHTS);
        MAKE_OLD_ARRAY(PackedInt32Array, old_idx, Mesh::ARRAY_INDEX);
        #undef MAKE_OLD_ARRAY

        arrays.clear();

        if (surf.warp_along_curve) {
            old_idx.reverse(); // not sure why necessary
        }

        // No negative z values allowed, offset x and y by desired amounts
        double min_z = INFINITY;
        for (const Vector3 &vert : old_verts) {
            min_z = Math::min(double(vert.z), min_z);
        }
        for (Vector3 &vert : old_verts) {
            vert.x += surf.offset.x;
            vert.y += surf.offset.y;
            vert.z -= min_z;
        }

        uint64_t count = 2;
        uint64_t max_count = _get_max_count();
        switch (surf.distribution) {
            case Distribution::DISTRIBUTE_BY_COUNT: {
                count = Math::clamp(surf.count, 2ULL, max_count);
            } break;
            case Distribution::DISTRIBUTE_BY_MODEL_LENGTH: {
                count = max_count;
            } break;
            default:
                UtilityFunctions::push_error("Distribution type is incorrect.");
                continue;
        }

        double z_stretch = 1.0;
        double z = 0.0;
        switch (surf.alignment) {
            case Alignment::ALIGN_STRETCH: {
                double real_l = mesh_l * count;
                z_stretch = baked_l / real_l;
                z = 0.0;
            } break;
            case Alignment::ALIGN_FROM_START: {
                z_stretch = 1.0;
                z = 0.0;
            } break;
            case Alignment::ALIGN_CENTERED: {
                z_stretch = 1.0;
                double real_l = mesh_l * count;
                z = (baked_l - real_l) / 2.0;
            } break;
            case Alignment::ALIGN_FROM_END: {
                z_stretch = 1.0;
                double real_l = mesh_l * count;
                z = baked_l - real_l;
            } break;
            default:
                UtilityFunctions::push_error("Alignment type is incorrect.");
                continue;
        }
        z_stretch = Math::max(z_stretch, 1.0);
        z = Math::clamp(z, 0.0, baked_l);

        uint64_t new_size = count * old_verts.size();
        PackedVector3Array new_verts; new_verts.resize(new_size);
        PackedVector3Array new_norms; new_norms.resize(new_size);
        PackedFloat32Array new_tang; new_tang.resize(new_size*4);
        PackedVector2Array new_uv1;
        PackedVector2Array new_uv2;
        PackedColorArray new_colors;
        PackedByteArray new_custom0;
        PackedByteArray new_custom1;
        PackedByteArray new_custom2;
        PackedByteArray new_custom3;
        PackedInt32Array new_bones;
        PackedFloat32Array new_weights;
        PackedInt32Array new_idx;

        uint64_t k = 0; // overall index into new arrays

        for (uint64_t idx_count = 0; idx_count < count; ++idx_count) {
            Transform3D transform;
            if (!surf.warp_along_curve) {
                double z_offset = z + 0.5*z_stretch*mesh_l;
                transform = curve->sample_baked_with_rotation(z_offset, surf.cubic, surf.tilt);
            }

            for (uint64_t idx_vert = 0; idx_vert < old_verts.size(); ++idx_vert) {
                if (surf.warp_along_curve) {
                    double z_offset = z + z_stretch*double(old_verts[idx_vert].z);
                    transform = curve->sample_baked_with_rotation(z_offset, surf.cubic, surf.tilt);
                    transform.basis.set_column(2, Vector3(0.0, 0.0, 0.0));
                }

                new_verts[k] = transform.xform(old_verts[idx_vert]);
                if (has_column[Mesh::ARRAY_NORMAL]) {
                    new_norms[k] = transform.basis.xform(old_norms[idx_vert]);
                }
                if (has_column[Mesh::ARRAY_TANGENT]) {
                    Vector3 tang = transform.basis.xform(Vector3(old_tang[idx_vert*4], old_tang[idx_vert*4 + 1], old_tang[idx_vert*4 + 2]));
                    new_tang[k*4] = tang.x;
                    new_tang[k*4 + 1] = tang.y;
                    new_tang[k*4 + 2] = tang.z;
                    new_tang[k*4 + 3] = old_tang[idx_vert*4 + 3];
                }

                k++;
            }

            new_uv1.append_array(old_uv1);
            new_uv2.append_array(old_uv2);
            new_colors.append_array(old_colors);
            new_custom0.append_array(old_custom0);
            new_custom1.append_array(old_custom1);
            new_custom2.append_array(old_custom2);
            new_custom3.append_array(old_custom3);
            new_bones.append_array(old_bones);
            new_weights.append_array(old_weights);
            new_idx.append_array(old_idx);
            for (int32_t &idx : old_idx) {
                idx += old_verts.size(); // increase the indices by size each loop
            }
            
            z += mesh_l*z_stretch;
        }

        arrays.resize(Mesh::ARRAY_MAX);
        arrays.fill(Variant());
        arrays[Mesh::ARRAY_VERTEX] = new_verts;
        #define MAKE_NEW_ARRAY(m_index, m_name) arrays[m_index] = m_name.is_empty() ? Variant() : m_name
        MAKE_NEW_ARRAY(Mesh::ARRAY_NORMAL, new_norms);
        MAKE_NEW_ARRAY(Mesh::ARRAY_TANGENT, new_tang);
        MAKE_NEW_ARRAY(Mesh::ARRAY_TEX_UV, new_uv1);
        MAKE_NEW_ARRAY(Mesh::ARRAY_TEX_UV2, new_uv2);
        MAKE_NEW_ARRAY(Mesh::ARRAY_COLOR, new_colors);
        MAKE_NEW_ARRAY(Mesh::ARRAY_CUSTOM0, new_custom0);
        MAKE_NEW_ARRAY(Mesh::ARRAY_CUSTOM1, new_custom1);
        MAKE_NEW_ARRAY(Mesh::ARRAY_CUSTOM2, new_custom2);
        MAKE_NEW_ARRAY(Mesh::ARRAY_CUSTOM3, new_custom3);
        MAKE_NEW_ARRAY(Mesh::ARRAY_BONES, new_bones);
        MAKE_NEW_ARRAY(Mesh::ARRAY_WEIGHTS, new_weights);
        MAKE_NEW_ARRAY(Mesh::ARRAY_INDEX, new_idx);
        #undef MAKE_NEW_ARRAY

        Mesh::PrimitiveType primitives = Mesh::PRIMITIVE_TRIANGLES;
        Ref<ArrayMesh> arr_mesh = cast_to<ArrayMesh>(source_mesh.ptr());
        if (arr_mesh != nullptr) {
            primitives = arr_mesh->surface_get_primitive_type(idx_surf);
        }

        generated_mesh->add_surface_from_arrays(primitives, arrays, source_mesh->surface_get_blend_shape_arrays(idx_surf));
        generated_mesh->surface_set_material(idx_surf, source_mesh->surface_get_material(idx_surf));
    }
}

bool PathMesh3D::_are_any_dirty() const {
    for (const SurfaceData &surf : surfaces) {
        if (surf.dirty) {
            return true;
        }
    }

    return false;
}

Pair<uint64_t, String> PathMesh3D::_decode_dynamic_propname(const StringName &p_name) const {
    PackedStringArray prop_path = p_name.split("/");
    if (prop_path.size() != 2) {
        return Pair<uint64_t, String>();
    }

    uint64_t surf_idx = prop_path[0].trim_prefix("surface_").to_int();
    String sub_name = prop_path[1];
    return Pair(surf_idx, sub_name);
}

uint64_t PathMesh3D::_get_max_count() const {
    if (source_mesh.is_valid() && path3d != nullptr && path3d->get_curve().is_valid()) {
        return path3d->get_curve()->get_baked_length() / source_mesh->get_aabb().size.z;
    } else {
        return 100;
    }
}

void PathMesh3D::_on_mesh_changed() {
    if (source_mesh.is_valid()) {
        surfaces.resize(source_mesh->get_surface_count());
    } else {
        surfaces.clear();
    }
    emit_signal("mesh_changed");
    queue_rebuild();
    notify_property_list_changed();
}

void PathMesh3D::_on_curve_changed() {
    emit_signal("curve_changed");
    queue_rebuild();
}