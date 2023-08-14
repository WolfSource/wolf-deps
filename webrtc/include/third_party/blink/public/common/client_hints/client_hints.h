// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_CLIENT_HINTS_CLIENT_HINTS_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_CLIENT_HINTS_CLIENT_HINTS_H_

#include <stddef.h>
#include <string>

#include "base/containers/flat_map.h"
#include "services/network/public/mojom/web_client_hints_types.mojom-shared.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/common_export.h"
#include "third_party/blink/public/mojom/permissions_policy/permissions_policy_feature.mojom.h"

class GURL;

namespace blink {

class PermissionsPolicy;

using ClientHintToPolicyFeatureMap =
    base::flat_map<network::mojom::WebClientHintsType,
                   mojom::PermissionsPolicyFeature>;

// Mapping from WebClientHintsType to the corresponding Permissions-Policy (e.g.
// kDpr => kClientHintsDPR). The order matches the header mapping and the enum
// order in services/network/public/mojom/web_client_hints_types.mojom
BLINK_COMMON_EXPORT const ClientHintToPolicyFeatureMap&
GetClientHintToPolicyFeatureMap();

// Mapping from WebEffectiveConnectionType to the header value. This value is
// sent to the origins and is returned by the JavaScript API. The ordering
// should match the ordering in //net/nqe/effective_connection_type.h and
// public/platform/WebEffectiveConnectionType.h.
// This array should be updated if either of the enums in
// effective_connection_type.h or WebEffectiveConnectionType.h are updated.
BLINK_COMMON_EXPORT extern const char* const
    kWebEffectiveConnectionTypeMapping[];

BLINK_COMMON_EXPORT extern const size_t kWebEffectiveConnectionTypeMappingCount;

// Indicates that a hint is sent by default, regardless of an opt-in.
BLINK_COMMON_EXPORT
bool IsClientHintSentByDefault(network::mojom::WebClientHintsType type);

// Add a list of Client Hints headers to be removed to the output vector, based
// on Permissions Policy and the url's origin.
BLINK_COMMON_EXPORT void FindClientHintsToRemove(
    const PermissionsPolicy* permissions_policy,
    const GURL& url,
    std::vector<std::string>* removed_headers);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_CLIENT_HINTS_CLIENT_HINTS_H_
