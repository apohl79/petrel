-- Request handling entry point
function handle_advis(request, response)
   return response
end

-- Request handling entry point
function handle_advis2(request, response)
   -- parse url
   local parsed_req = adtech_parse_url(request.path)
   if parsed_req == nil then
      return bad_request(response, "bad url")
   end
   -- validate advis parameters
   local success, err = validate(parsed_req)
   if not success then
      return bad_request(response, err)
   end
   -- parse events
   local events
   local values
   if parsed_req.params.eventids ~= nil and parsed_req.params.eventvals ~= nil then
      events = string.split(parsed_req.params.eventids, ",")
      values = string.split(parsed_req.params.eventvals, ",")
      if events.count ~= values.count then
         return bad_request(response, "number of events does not match number values")
      end
   else
      return bad_request(response, "missing EventIds/EventVals parameters")
   end
   -- parse refseqid
   local refseq32 = parsed_req.params.refseqid or parsed_req.params.refsequenceid
   local refseq64 = parsed_req.params.refseqid2
   if refseq64 ~= nil then
      refseq64 = base64.decode(refseq64)
   end
   -- campaign network
   if parsed_req.params.CampaignNetworkId == nil then
      parsed_req.params.CampaignNetworkId = -1
      parsed_req.params.CampaignSubNetworkId = -1
   end
   -- FIXME: implement userid handling
   userid = ""
   -- FIXME: implement ip hashing
   ip = 0
   -- FIXME: implement adserver ip/farm
   adserver_ip = 0
   adserver_farm = 0
   -- FIXME: implement sequenceid
   sequence_id = 0
   -- create avro record
   local status, err = pcall(function()
         for i = 1, events.count do
            event = math.tointeger(events[i])
            value = math.tointeger(values[i])
            if event >= 900 and event <= 999 then
               print(LOG_DEBUG, "Creating avro record for:", "event", event, "value", value)
               packet = adtech_avro()
               packet:create_packet(advis_avro_schema)
               ---- Header part
               --packet:get_record("SGSHeader")
               --packet:write_int32("VersionID", 1)
               --packet:write_int32("RecordType", record_types.Advisibility)
               --packet:write_int32("TimeStamp", request.timestamp)
               --packet:write_int32("GMTOffset", 0)
               --packet:write_int32("SequenceID", sequence_id)
               --packet:write_record("SGSHeader")
               ---- Advis part
               --packet:get_record("AdVisibilityPacket")
               --packet:write_int32("PlcNetworkId", parsed_req.networkid)
               --packet:write_int32("PlcSubNetworkId", parsed_req.subnetworkid)
               --packet:write_int32("CampaignNetworkId", parsed_req.params.CampaignNetworkId)
               --packet:write_int32("CampaignSubNetworkId", parsed_req.params.CampaignSubNetworkId)
               --packet:write_int64("RefSequenceId", refseq64)
               --packet:write_int32("PlacementId", parsed_req.placementid)
               --packet:write_int32("CampaignId", parsed_req.params.adid)
               --packet:write_int32("BannerNumber", parsed_req.params.bnid)
               --packet:write_int32("MediaId", parsed_req.params.assetid)
               --packet:write_string("UserId", userid)
               --packet:write_int32("IpAddress", ip)
               --packet:write_int32("AdServerIp", adserver_ip)
               --packet:write_int32("AdServerFarmId", adserver_farm)
               --packet:write_int32("EventTypeId", event)
               --packet:write_int32("EventValue", value)
               --packet:write_record("AdVisibilityPacket")
               -- TODO: send record somewhere
            else
               print(LOG_DEBUG, "Skipping out of range event:", "event", event, "value", value)               
            end
         end
   end)
   if err then
      print(LOG_CRIT, "failed to create avro record:", err)
   end
   -- send OK response back
   return response
end

function validate(parsed_req)
   if parsed_req.subnetworkid == nil then
      return false, "subnetworkid missing"
   end
   if parsed_req.params.adid == nil then
      return false, "adid missing"
   end
   if parsed_req.params.assetid == nil then
      return false, "assetid missing"
   end
   if parsed_req.params.bnid == nil then
      return false, "bnid missing"
   end
   if parsed_req.params.refseqid2 == nil then
      return false, "refseq2 missing"
   end
   return true
end

function bad_request(response, reason)
   print(LOG_DEBUG, "bad request:", reason)
   response.status = 400
   return response
end

