// +build !ignore_autogenerated

// This file was autogenerated by openapi-gen. Do not edit it manually!

package v1beta1

import (
	spec "github.com/go-openapi/spec"
	common "k8s.io/kube-openapi/pkg/common"
)

func GetOpenAPIDefinitions(ref common.ReferenceCallback) map[string]common.OpenAPIDefinition {
	return map[string]common.OpenAPIDefinition{
		"./pkg/apis/openj9/v1beta1.RuntimeComponent":            schema_pkg_apis_appstacks_v1beta1_RuntimeComponent(ref),
		"./pkg/apis/openj9/v1beta1.RuntimeComponentAffinity":    schema_pkg_apis_appstacks_v1beta1_RuntimeComponentAffinity(ref),
		"./pkg/apis/openj9/v1beta1.RuntimeComponentAutoScaling": schema_pkg_apis_appstacks_v1beta1_RuntimeComponentAutoScaling(ref),
		"./pkg/apis/openj9/v1beta1.RuntimeComponentRoute":       schema_pkg_apis_appstacks_v1beta1_RuntimeComponentRoute(ref),
		"./pkg/apis/openj9/v1beta1.RuntimeComponentService":     schema_pkg_apis_appstacks_v1beta1_RuntimeComponentService(ref),
		"./pkg/apis/openj9/v1beta1.RuntimeComponentSpec":        schema_pkg_apis_appstacks_v1beta1_RuntimeComponentSpec(ref),
		"./pkg/apis/openj9/v1beta1.RuntimeComponentStatus":      schema_pkg_apis_appstacks_v1beta1_RuntimeComponentStatus(ref),
		"./pkg/apis/openj9/v1beta1.ServiceBindingConsumes":      schema_pkg_apis_appstacks_v1beta1_ServiceBindingConsumes(ref),
		"./pkg/apis/openj9/v1beta1.ServiceBindingProvides":      schema_pkg_apis_appstacks_v1beta1_ServiceBindingProvides(ref),
		"./pkg/apis/openj9/v1beta1.StatusCondition":             schema_pkg_apis_appstacks_v1beta1_StatusCondition(ref),
	}
}

func schema_pkg_apis_appstacks_v1beta1_RuntimeComponent(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "RuntimeComponent is the Schema for the runtimecomponents API",
				Type:        []string{"object"},
				Properties: map[string]spec.Schema{
					"kind": {
						SchemaProps: spec.SchemaProps{
							Description: "Kind is a string value representing the REST resource this object represents. Servers may infer this from the endpoint the client submits requests to. Cannot be updated. In CamelCase. More info: https://git.k8s.io/community/contributors/devel/sig-architecture/api-conventions.md#types-kinds",
							Type:        []string{"string"},
							Format:      "",
						},
					},
					"apiVersion": {
						SchemaProps: spec.SchemaProps{
							Description: "APIVersion defines the versioned schema of this representation of an object. Servers should convert recognized schemas to the latest internal value, and may reject unrecognized values. More info: https://git.k8s.io/community/contributors/devel/sig-architecture/api-conventions.md#resources",
							Type:        []string{"string"},
							Format:      "",
						},
					},
					"metadata": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("k8s.io/apimachinery/pkg/apis/meta/v1.ObjectMeta"),
						},
					},
					"spec": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.RuntimeComponentSpec"),
						},
					},
					"status": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.RuntimeComponentStatus"),
						},
					},
				},
			},
		},
		Dependencies: []string{
			"./pkg/apis/openj9/v1beta1.RuntimeComponentSpec", "./pkg/apis/openj9/v1beta1.RuntimeComponentStatus", "k8s.io/apimachinery/pkg/apis/meta/v1.ObjectMeta"},
	}
}

func schema_pkg_apis_appstacks_v1beta1_RuntimeComponentAffinity(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "RuntimeComponentAffinity deployment affinity settings",
				Type:        []string{"object"},
				Properties: map[string]spec.Schema{
					"nodeAffinity": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("k8s.io/api/core/v1.NodeAffinity"),
						},
					},
					"podAffinity": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("k8s.io/api/core/v1.PodAffinity"),
						},
					},
					"podAntiAffinity": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("k8s.io/api/core/v1.PodAntiAffinity"),
						},
					},
					"architecture": {
						VendorExtensible: spec.VendorExtensible{
							Extensions: spec.Extensions{
								"x-kubernetes-list-type": "set",
							},
						},
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Type:   []string{"string"},
										Format: "",
									},
								},
							},
						},
					},
					"nodeAffinityLabels": {
						SchemaProps: spec.SchemaProps{
							Type: []string{"object"},
							AdditionalProperties: &spec.SchemaOrBool{
								Allows: true,
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Type:   []string{"string"},
										Format: "",
									},
								},
							},
						},
					},
				},
			},
		},
		Dependencies: []string{
			"k8s.io/api/core/v1.NodeAffinity", "k8s.io/api/core/v1.PodAffinity", "k8s.io/api/core/v1.PodAntiAffinity"},
	}
}

func schema_pkg_apis_appstacks_v1beta1_RuntimeComponentAutoScaling(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "RuntimeComponentAutoScaling ...",
				Type:        []string{"object"},
				Properties: map[string]spec.Schema{
					"targetCPUUtilizationPercentage": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"integer"},
							Format: "int32",
						},
					},
					"minReplicas": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"integer"},
							Format: "int32",
						},
					},
					"maxReplicas": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"integer"},
							Format: "int32",
						},
					},
				},
			},
		},
	}
}

func schema_pkg_apis_appstacks_v1beta1_RuntimeComponentRoute(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "RuntimeComponentRoute ...",
				Type:        []string{"object"},
				Properties: map[string]spec.Schema{
					"annotations": {
						SchemaProps: spec.SchemaProps{
							Type: []string{"object"},
							AdditionalProperties: &spec.SchemaOrBool{
								Allows: true,
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Type:   []string{"string"},
										Format: "",
									},
								},
							},
						},
					},
					"termination": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"insecureEdgeTerminationPolicy": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"certificate": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.Certificate"),
						},
					},
					"certificateSecretRef": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"host": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"path": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
				},
			},
		},
		Dependencies: []string{
			"./pkg/apis/openj9/v1beta1.Certificate"},
	}
}

func schema_pkg_apis_appstacks_v1beta1_RuntimeComponentService(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "RuntimeComponentService ...",
				Type:        []string{"object"},
				Properties: map[string]spec.Schema{
					"type": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"port": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"integer"},
							Format: "int32",
						},
					},
					"targetPort": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"integer"},
							Format: "int32",
						},
					},
					"nodePort": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"integer"},
							Format: "int32",
						},
					},
					"portName": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"ports": {
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Ref: ref("k8s.io/api/core/v1.ServicePort"),
									},
								},
							},
						},
					},
					"annotations": {
						SchemaProps: spec.SchemaProps{
							Type: []string{"object"},
							AdditionalProperties: &spec.SchemaOrBool{
								Allows: true,
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Type:   []string{"string"},
										Format: "",
									},
								},
							},
						},
					},
					"consumes": {
						VendorExtensible: spec.VendorExtensible{
							Extensions: spec.Extensions{
								"x-kubernetes-list-type": "atomic",
							},
						},
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Ref: ref("./pkg/apis/openj9/v1beta1.ServiceBindingConsumes"),
									},
								},
							},
						},
					},
					"provides": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.ServiceBindingProvides"),
						},
					},
					"certificate": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.Certificate"),
						},
					},
					"certificateSecretRef": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
				},
			},
		},
		Dependencies: []string{
			"./pkg/apis/openj9/v1beta1.Certificate", "./pkg/apis/openj9/v1beta1.ServiceBindingConsumes", "./pkg/apis/openj9/v1beta1.ServiceBindingProvides", "k8s.io/api/core/v1.ServicePort"},
	}
}

func schema_pkg_apis_appstacks_v1beta1_RuntimeComponentSpec(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "RuntimeComponentSpec defines the desired state of RuntimeComponent",
				Type:        []string{"object"},
				Properties: map[string]spec.Schema{
					"version": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"applicationImage": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"replicas": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"integer"},
							Format: "int32",
						},
					},
					"autoscaling": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.RuntimeComponentAutoScaling"),
						},
					},
					"pullPolicy": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"pullSecret": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"volumes": {
						VendorExtensible: spec.VendorExtensible{
							Extensions: spec.Extensions{
								"x-kubernetes-list-map-keys": "name",
								"x-kubernetes-list-type":     "map",
							},
						},
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Ref: ref("k8s.io/api/core/v1.Volume"),
									},
								},
							},
						},
					},
					"volumeMounts": {
						VendorExtensible: spec.VendorExtensible{
							Extensions: spec.Extensions{
								"x-kubernetes-list-type": "atomic",
							},
						},
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Ref: ref("k8s.io/api/core/v1.VolumeMount"),
									},
								},
							},
						},
					},
					"resourceConstraints": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("k8s.io/api/core/v1.ResourceRequirements"),
						},
					},
					"readinessProbe": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("k8s.io/api/core/v1.Probe"),
						},
					},
					"livenessProbe": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("k8s.io/api/core/v1.Probe"),
						},
					},
					"service": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.RuntimeComponentService"),
						},
					},
					"expose": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"boolean"},
							Format: "",
						},
					},
					"envFrom": {
						VendorExtensible: spec.VendorExtensible{
							Extensions: spec.Extensions{
								"x-kubernetes-list-type": "atomic",
							},
						},
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Ref: ref("k8s.io/api/core/v1.EnvFromSource"),
									},
								},
							},
						},
					},
					"env": {
						VendorExtensible: spec.VendorExtensible{
							Extensions: spec.Extensions{
								"x-kubernetes-list-map-keys": "name",
								"x-kubernetes-list-type":     "map",
							},
						},
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Ref: ref("k8s.io/api/core/v1.EnvVar"),
									},
								},
							},
						},
					},
					"serviceAccountName": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"architecture": {
						VendorExtensible: spec.VendorExtensible{
							Extensions: spec.Extensions{
								"x-kubernetes-list-type": "set",
							},
						},
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Type:   []string{"string"},
										Format: "",
									},
								},
							},
						},
					},
					"storage": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.RuntimeComponentStorage"),
						},
					},
					"createKnativeService": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"boolean"},
							Format: "",
						},
					},
					"monitoring": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.RuntimeComponentMonitoring"),
						},
					},
					"createAppDefinition": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"boolean"},
							Format: "",
						},
					},
					"applicationName": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"initContainers": {
						VendorExtensible: spec.VendorExtensible{
							Extensions: spec.Extensions{
								"x-kubernetes-list-map-keys": "name",
								"x-kubernetes-list-type":     "map",
							},
						},
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Ref: ref("k8s.io/api/core/v1.Container"),
									},
								},
							},
						},
					},
					"sidecarContainers": {
						VendorExtensible: spec.VendorExtensible{
							Extensions: spec.Extensions{
								"x-kubernetes-list-map-keys": "name",
								"x-kubernetes-list-type":     "map",
							},
						},
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Ref: ref("k8s.io/api/core/v1.Container"),
									},
								},
							},
						},
					},
					"route": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.RuntimeComponentRoute"),
						},
					},
					"bindings": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.RuntimeComponentBindings"),
						},
					},
					"affinity": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.RuntimeComponentAffinity"),
						},
					},
				},
				Required: []string{"applicationImage"},
			},
		},
		Dependencies: []string{
			"./pkg/apis/openj9/v1beta1.RuntimeComponentAffinity", "./pkg/apis/openj9/v1beta1.RuntimeComponentAutoScaling", "./pkg/apis/openj9/v1beta1.RuntimeComponentBindings", "./pkg/apis/openj9/v1beta1.RuntimeComponentMonitoring", "./pkg/apis/openj9/v1beta1.RuntimeComponentRoute", "./pkg/apis/openj9/v1beta1.RuntimeComponentService", "./pkg/apis/openj9/v1beta1.RuntimeComponentStorage", "k8s.io/api/core/v1.Container", "k8s.io/api/core/v1.EnvFromSource", "k8s.io/api/core/v1.EnvVar", "k8s.io/api/core/v1.Probe", "k8s.io/api/core/v1.ResourceRequirements", "k8s.io/api/core/v1.Volume", "k8s.io/api/core/v1.VolumeMount"},
	}
}

func schema_pkg_apis_appstacks_v1beta1_RuntimeComponentStatus(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "RuntimeComponentStatus defines the observed state of RuntimeComponent",
				Type:        []string{"object"},
				Properties: map[string]spec.Schema{
					"conditions": {
						VendorExtensible: spec.VendorExtensible{
							Extensions: spec.Extensions{
								"x-kubernetes-list-type": "atomic",
							},
						},
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Ref: ref("./pkg/apis/openj9/v1beta1.StatusCondition"),
									},
								},
							},
						},
					},
					"consumedServices": {
						SchemaProps: spec.SchemaProps{
							Type: []string{"object"},
							AdditionalProperties: &spec.SchemaOrBool{
								Allows: true,
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Type: []string{"array"},
										Items: &spec.SchemaOrArray{
											Schema: &spec.Schema{
												SchemaProps: spec.SchemaProps{
													Type:   []string{"string"},
													Format: "",
												},
											},
										},
									},
								},
							},
						},
					},
					"resolvedBindings": {
						VendorExtensible: spec.VendorExtensible{
							Extensions: spec.Extensions{
								"x-kubernetes-list-type": "set",
							},
						},
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Type:   []string{"string"},
										Format: "",
									},
								},
							},
						},
					},
					"imageReference": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
				},
			},
		},
		Dependencies: []string{
			"./pkg/apis/openj9/v1beta1.StatusCondition"},
	}
}

func schema_pkg_apis_appstacks_v1beta1_ServiceBindingConsumes(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "ServiceBindingConsumes represents a service to be consumed",
				Type:        []string{"object"},
				Properties: map[string]spec.Schema{
					"name": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"namespace": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"category": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"mountPath": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
				},
				Required: []string{"name", "category"},
			},
		},
	}
}

func schema_pkg_apis_appstacks_v1beta1_ServiceBindingProvides(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "ServiceBindingProvides represents information about",
				Type:        []string{"object"},
				Properties: map[string]spec.Schema{
					"category": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"context": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"protocol": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"auth": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/openj9/v1beta1.ServiceBindingAuth"),
						},
					},
				},
				Required: []string{"category"},
			},
		},
		Dependencies: []string{
			"./pkg/apis/openj9/v1beta1.ServiceBindingAuth"},
	}
}

func schema_pkg_apis_appstacks_v1beta1_StatusCondition(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "StatusCondition ...",
				Type:        []string{"object"},
				Properties: map[string]spec.Schema{
					"lastTransitionTime": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("k8s.io/apimachinery/pkg/apis/meta/v1.Time"),
						},
					},
					"lastUpdateTime": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("k8s.io/apimachinery/pkg/apis/meta/v1.Time"),
						},
					},
					"reason": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"message": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"status": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"type": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
				},
			},
		},
		Dependencies: []string{
			"k8s.io/apimachinery/pkg/apis/meta/v1.Time"},
	}
}
