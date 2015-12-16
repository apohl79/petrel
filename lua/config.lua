-- example main config
main_config = {
   key1 = "value",
   key2 = "value2"
}
-- example avro config
avro_config = {
   advis_endpoints = {
      "http://host1:12345",
      "http://host2:12345"
   },
   key1 = "value"
}

advis_avro_schema = [[
{
  "type" : "record",
  "name" : "DataAvroPacket",
  "fields" : [ {
    "name" : "SGSHeader",
    "type" : {
      "type" : "record",
      "name" : "SGSHeader",
      "fields" : [ {
        "name" : "VersionID",
        "type" : "int"
      }, {
        "name" : "SequenceID",
        "type" : "long"
      }, {
        "name" : "RecordType",
        "type" : "int"
      }, {
        "name" : "TimeStamp",
        "type" : "int"
      }, {
        "name" : "GMTOffset",
        "type" : "int"
      } ]
    }
  }, {
    "name" : "AdVisibilityPacket",
    "type" : [ "null", {
      "type" : "record",
      "name" : "AdVisibilityPacket",
      "fields" : [ {
        "name" : "PlcNetworkId",
        "type" : "int"
      }, {
        "name" : "PlcSubNetworkId",
        "type" : "int"
      }, {
        "name" : "CampaignNetworkId",
        "type" : "int"
      }, {
        "name" : "CampaignSubNetworkId",
        "type" : "int"
      }, {
        "name" : "RefSequenceId",
        "type" : "long"
      }, {
        "name" : "PlacementId",
        "type" : "int"
      }, {
        "name" : "CampaignId",
        "type" : "int"
      }, {
        "name" : "BannerNumber",
        "type" : "int"
      }, {
        "name" : "MediaId",
        "type" : "int"
      }, {
        "name" : "UserId",
        "type" : "string"
      }, {
        "name" : "IpAddress",
        "type" : "int"
      }, {
        "name" : "AdServerIp",
        "type" : "int"
      }, {
        "name" : "AdServerFarmId",
        "type" : "int"
      }, {
        "name" : "EventTypeId",
        "type" : "int"
      }, {
        "name" : "EventValue",
        "type" : "int"
      } ]
    } ]
  } ]
}
]]
