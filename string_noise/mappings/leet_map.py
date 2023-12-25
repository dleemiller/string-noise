from .mapper import Mapper

resource_package = 'string_noise.mappings.default'
resource_path = 'leet_map.json'

leet = Mapper.load(resource_package, resource_path)

